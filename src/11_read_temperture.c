/**
 * I2C 通信を使用して温度センサーから温度データを読み取るプログラム
 * 読み取ったデータはLCDディスプレイに表示されます。
 * データの読み取り間隔は 5 秒ごとです。
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define LCD_ADDR 0x3E
#define TEMP_SENSOR_ADDR 0x48
#define I2C_DEV "/dev/i2c-1"

static const int CHARS_PER_LINE = 8;
static const int DISPLAY_LINES = 2;

static int display_chars = CHARS_PER_LINE * DISPLAY_LINES;
static int position = 0;
static int line = 0;

void lcd_cmd(int, unsigned char);
void lcd_data(int, unsigned char);
void lcd_init(int);
void lcd_clear(int);
void lcd_newline(int);
void lcd_print(int, const char *);
void lcd_write_char(int, const char);

int read_temperature(int, float *);

static int ret = 0;

int main() {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("i2c open");
        ret = 1;
        goto cleanup;
    }

    // LCD は初期化が必要なため最初に呼び出し
    if (ioctl(fd, I2C_SLAVE, LCD_ADDR) < 0) {
        perror("i2c ioctl LCD for init");
        ret = 1;
        goto cleanup;
    }

    lcd_init(fd);

    while (1) {
        // 温度センサーのアドレス設定
        if (ioctl(fd, I2C_SLAVE, TEMP_SENSOR_ADDR) < 0) {
            perror("i2c ioctl TEMP_SENSOR");
            ret = 1;
            goto cleanup;
        }

        // 温度データの読み取り
        float temp;
        if (read_temperature(fd, &temp) < 0) {
            ret = 1;
            goto cleanup;
        }

        // LCDのアドレス設定
        if (ioctl(fd, I2C_SLAVE, LCD_ADDR) < 0) {
            perror("i2c ioctl LCD");
            ret = 1;
            goto cleanup;
        }

        // LCDに温度表示
        char temp_str[32];
        snprintf(temp_str, sizeof(temp_str), "Temp: %.2f C", temp);
        lcd_clear(fd);
        lcd_print(fd, temp_str);

        usleep(5000000); // 5秒待機
    }

cleanup:
    close(fd);
    return ret;
}

// ---------------------------
// 温度センサー 関数定義
// ---------------------------

int read_temperature(int fd, float *temp) {
    uint8_t reg = 0x00; // 温度レジスタ
    if (write(fd, &reg, 1) < 0) {
        perror("i2c read temperature when write sequence");
        return -1;
    }

    uint8_t buf[2];
    if (read(fd, buf, 2) < 0) {
        perror("i2c read temperature data");
        return -1;
    }

    int16_t raw = (buf[0] << 8) | buf[1];
    *temp = raw / 128.0;

    return 0;
}

// ---------------------------
// LCD 関数定義
// ---------------------------

void lcd_cmd(int fd, unsigned char cmd) {
    unsigned char buffer[2] = {0x00, cmd};
    write(fd, buffer, 2);
    usleep(30);
}

// LCD へのデータ送信
void lcd_data(int fd, unsigned char data) {
    unsigned char buffer[2] = {0x40, data};
    write(fd, buffer, 2);
    usleep(30);
}

// LCD 初期化
void lcd_init(int fd) {
    usleep(40000); // 起動待ち
    lcd_cmd(fd, 0x38); // Function set
    lcd_cmd(fd, 0x39); // Function set (Extended)
    lcd_cmd(fd, 0x14); // Internal OSC frequency
    lcd_cmd(fd, 0x73); // Contrast set (lower 4 bits)
    lcd_cmd(fd, 0x56); // Power/icon/contrast control
    lcd_cmd(fd, 0x6c); // Follower control
    usleep(200000);    // 待ち時間
    lcd_cmd(fd, 0x38); // Function set
    lcd_cmd(fd, 0x0c); // Display ON
    lcd_cmd(fd, 0x01); // Clear display
    usleep(2000);
}

// LCDの表示を空にする
void lcd_clear(int fd) {
    position = 0;
    line = 0;
    lcd_cmd(fd, 0x01);
    usleep(1000);
}

// 改行表示
// 2行書き込み済みの場合、LCDの表示を空にする
void lcd_newline(int fd) {
    if (line == (DISPLAY_LINES - 1)) {
        lcd_clear(fd);
        return;
    }

    // 行変更
    line += 1;
    position = CHARS_PER_LINE * line;
    lcd_cmd(fd, 0xc0);
    usleep(1000);
}

// 文字列表示
void lcd_print(int fd, const char *str) {
    while (*str) {
        lcd_write_char(fd, *str++);
    }
}

// 表示範囲を意識した文字出力
void lcd_write_char(int fd, const char ch) {
    if (position == display_chars) {
        lcd_clear(fd);
    } else if (position == CHARS_PER_LINE * (line + 1)) {
        lcd_newline(fd);
    }
    lcd_data(fd, ch);
    position += 1;
}