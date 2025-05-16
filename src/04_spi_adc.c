#include <stdio.h>  // printf関数の定義ファイル
#include <stdlib.h> // exit関数の定義ファイル
#include <stdint.h> // uint8_tの定義ファイル
#include <fcntl.h>  // open関数の定義ファイル
#include <unistd.h>	// sleep関数の定義ファイル
#include <linux/spi/spidev.h> // SPIの定義ファイル
#include <sys/ioctl.h> // ioctl関数の定義ファイル
#include <gpiod.h>	// gpiodの定義ファイル


#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED 1000000 // 1 MHz

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
    // SPIデバイスのオープン
    int fd = open(SPI_DEVICE, O_RDWR); // O_RDWR = Read/Write
    if (fd < 0) {
        perror("open");
        return 1;
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

    for (int i = 0; i < 30; i++) {
        int adc = read_mcp3208(fd, 0); // CH0
        if (adc >= 0)
            printf("ADC CH0 Value: %d\n", adc); // 0 - 4095
        usleep(500000);
    }

    close(fd);
    return 0;
}
