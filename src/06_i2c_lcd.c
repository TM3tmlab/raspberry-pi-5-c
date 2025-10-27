#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <ifaddrs.h>

#define LCD_ADDR 0x3E
#define I2C_DEV "/dev/i2c-1"

static int lcd_fd;

void lcd_send(uint8_t control, uint8_t data) {
    uint8_t buf[2] = { control, data };
    write(lcd_fd, buf, 2);
}

void lcd_cmd(uint8_t cmd) {
    lcd_send(0x00, cmd);
    usleep(27); // 一部のコマンドはウェイトが必要
}

void lcd_data(uint8_t data) {
    lcd_send(0x40, data);
}

void lcd_init() {
    lcd_cmd(0x38); // Function set: 8bit, 2line, instruction table 0
    lcd_cmd(0x39); // Function set: IS=1
    lcd_cmd(0x14); // Bias set
    lcd_cmd(0x70); // Contrast set (lower 4 bits)
    lcd_cmd(0x56); // Power/ICON/Contrast set
    lcd_cmd(0x6C); // Follower control
    usleep(200000); // Wait for power on
    lcd_cmd(0x38); // Function set
    lcd_cmd(0x0C); // Display ON
    lcd_cmd(0x01); // Clear display
    usleep(2000);  // Clear display needs >1.08ms
}

void lcd_set_cursor(uint8_t line) {
    lcd_cmd(0x80 | (line ? 0x40 : 0x00));
}

void lcd_print(const char *str) {
    while (*str) lcd_data(*str++);
}

int main() {
    lcd_fd = open(I2C_DEV, O_RDWR);
    if (lcd_fd < 0) {
        perror("open");
        return 1;
    }
    if (ioctl(lcd_fd, I2C_SLAVE, LCD_ADDR) < 0) {
        perror("ioctl");
        return 1;
    }

    lcd_init();

    char *ip = "192.168.0.1";

    lcd_set_cursor(0);
    lcd_print("IP Address:");

    lcd_set_cursor(1);
    lcd_print(ip);

    close(lcd_fd);
    return 0;
}