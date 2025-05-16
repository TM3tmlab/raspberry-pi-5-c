#include <stdio.h>  // printf関数の定義ファイル
#include <stdlib.h> // exit関数の定義ファイル
#include <stdint.h> // uint8_tの定義ファイル
#include <fcntl.h>  // open関数の定義ファイル
#include <unistd.h>	// sleep関数の定義ファイル
#include <linux/spi/spidev.h> // SPIの定義ファイル
#include <sys/ioctl.h> // ioctl関数の定義ファイル
#include <gpiod.h>	// gpiodの定義ファイル
#include <signal.h> // 割り込みハンドラの定義ファイル

#define PROGRAM_NAME "adc-led"	// プログラム名の定義
#define CHIP_NAME "/dev/gpiochip0"    // GPIOチップの定義

#define GPIO_HIGH 1		// HIGH出力の定義
#define GPIO_LOW 0		// LOW出力の定義

#define GPIO_LED  25	// GPIO番号の定義

#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED 1000000 // 1 MHz

// 無限ループがあるので、SIGINT（Ctrl+C）で終了できるようにする
volatile int loop = 1; // ループ制御用フラグ
void sigint_handler(__attribute__((unused)) int signum)
{
    loop = 0; // ループを終了するためのフラグをセット
}

int read_mcp3008(int fd, uint8_t channel) {
    // MCP3008 の通信ビット列組み立て
    uint8_t tx[] = {
        0x01, // Start bit (1)
        0x01 << 7 | (channel << 4),               // SGL/DIFF=1, D2 D1, D0, followed by 4 zeros
        0x00                                           // Read 8 bits
    };
    uint8_t rx[3] = {0};

    // SPI通信構造体の準備
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3, // 3 * bits_per_word = 24 bits
        .delay_usecs = 0,
        .speed_hz = SPI_SPEED,
        .bits_per_word = 8,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("SPI_IOC_MESSAGE");
        return -1;
    }

    int value = ((rx[1] & 0x03) << 8) | rx[2]; // 10-bit result
    return value;
}

int read_mcp3208(int fd, uint8_t channel) {
    // MCP3208 の通信開始ビット列組み立て
    uint8_t tx[] = {
        0x06 | ((channel & 0x07) >> 2),               // Start bit (1), SGL/DIFF=1, D2
        ((channel & 0x03) << 6),                      // D1, D0, followed by 6 zeros
        0x00                                           // Read 8 bits
    };
    uint8_t rx[3] = {0};

    // SPI通信構造体の準備
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3, // 3 * bits_per_word = 24 bits
        .delay_usecs = 0,
        .speed_hz = SPI_SPEED,
        .bits_per_word = 8,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("SPI_IOC_MESSAGE");
        return -1;
    }

    int value = ((rx[1] & 0x0F) << 8) | rx[2]; // 12-bit result
    return value;
}

int main() {
    struct gpiod_chip *chip = NULL; // GPIOチップの構造体ポインタ
    struct gpiod_line *line_led = NULL; // GPIOラインの構造体ポインタ
    int fd = 0;

    // Ctrl+Cで終了できるようにシグナルハンドラを設定
    if ( signal(SIGINT, sigint_handler) == SIG_ERR ) {
        perror("signal");
        exit(100);
    }

    // SPIデバイスのオープン
    fd = open(SPI_DEVICE, O_RDWR); // O_RDWR = Read/Write
    if (fd < 0) {
        perror("open");
        goto cleanup;
    }

    // SPI mode 0
    uint8_t mode = SPI_MODE_0;
    ioctl(fd, SPI_IOC_WR_MODE, &mode);

    // bits per word
    uint8_t bits = 8;
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);

    // SPI speed
    uint32_t speed = SPI_SPEED;
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // GPIOチップのオープン
    chip = gpiod_chip_open(CHIP_NAME);
    if (!chip) {
        perror("Open chip");
        goto cleanup;
    }

    // GPIOラインの取得
    line_led = gpiod_chip_get_line(chip, GPIO_LED);
    if (!line_led) {
        perror("Get line");
        goto cleanup;
    }
    
    // GPIOラインに出力をリクエスト
    if (gpiod_line_request_output(line_led, PROGRAM_NAME, GPIO_LOW) < 0) {
        perror("gpiod_line_request_output");
        goto cleanup;
    }

    while (loop) {
        int adc = read_mcp3208(fd, 0); // CH0
        if (adc >= 500) {
            gpiod_line_set_value(line_led, GPIO_HIGH);
        } else {
            gpiod_line_set_value(line_led, GPIO_LOW);
        }
        usleep(500000);
    }

cleanup:
    if (fd) close(fd);
    if (line_led) gpiod_line_release(line_led);
    if (chip) gpiod_chip_close(chip);
    return 0;
}
