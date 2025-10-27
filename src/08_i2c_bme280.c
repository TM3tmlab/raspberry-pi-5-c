// 必要なヘッダは省略
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define I2C_DEV "/dev/i2c-1"
#define BME280_ADDR 0x76 // または 0x77

int read16_LE(int fd, uint8_t reg) {
    uint8_t buf[2];
    write(fd, &reg, 1);
    read(fd, buf, 2);
    return buf[0] | (buf[1] << 8);
}

uint16_t read_u16(int fd, uint8_t reg) {
    uint8_t buf[2];
    write(fd, &reg, 1);
    read(fd, buf, 2);
    return buf[0] | (buf[1] << 8);
}

int main() {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, BME280_ADDR) < 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    // 初期化
    uint8_t init[] = { 0xF2, 0x01 }; // ctrl_hum = 1x oversampling
    write(fd, init, 2);
    uint8_t ctrl_meas[] = { 0xF4, 0x27 }; // temp/press oversampling ×1, mode = normal
    write(fd, ctrl_meas, 2);
    usleep(100000); // wait for measurement

    // 読み出しバッファ
    uint8_t reg = 0xF7;
    uint8_t data[8];
    write(fd, &reg, 1);
    read(fd, data, 8);

    int32_t adc_P = ((data[0] << 12) | (data[1] << 4) | (data[2] >> 4));
    int32_t adc_T = ((data[3] << 12) | (data[4] << 4) | (data[5] >> 4));
    int32_t adc_H = (data[6] << 8) | data[7];

    // キャリブレーションデータ読み込み（簡略）
    uint16_t dig_T1 = read_u16(fd, 0x88);
    int16_t dig_T2 = read16_LE(fd, 0x8A);
    int16_t dig_T3 = read16_LE(fd, 0x8C);

    // 温度補正計算
    int32_t var1, var2, t_fine;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    T /= 100.0;

    printf("Temperature: %.2f °C\n", T);
    printf("Raw Pressure:    %d\n", adc_P);
    printf("Raw Humidity:    %d\n", adc_H);

    close(fd);
    return 0;
}
