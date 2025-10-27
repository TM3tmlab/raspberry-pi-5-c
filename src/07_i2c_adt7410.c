#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define I2C_DEV "/dev/i2c-1"
#define ADT7410_ADDR 0x48

int main() {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, ADT7410_ADDR) < 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    // Set configuration register to 16-bit mode
    uint8_t config[2] = { 0x03, 0x80 }; // 0x03 = config reg
    write(fd, config, 2);
    usleep(250000); // wait for conversion

    // Read temperature from register 0x00 (2 bytes)
    uint8_t reg = 0x00;
    write(fd, &reg, 1);
    uint8_t buf[2];
    read(fd, buf, 2);

    int16_t raw = (buf[0] << 8) | buf[1];
    raw >>= 3; // 13-bit mode: discard last 3 bits
    if (raw & 0x1000) raw |= 0xE000; // sign extension

    float temp = raw / 16.0;
    printf("Temperature: %.2f °C\n", temp);

    close(fd);
    return 0;
}
