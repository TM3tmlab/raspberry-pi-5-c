#include <gpiod.h>	// gpiodの定義ファイル
#include <unistd.h>	// sleep関数の定義ファイル
#include <stdio.h>  // printf関数の定義ファイル

#define PROGRAM_NAME "led-blink"	// プログラム名の定義
#define CHIP_NAME "/dev/gpiochip0"    // GPIOチップの定義

#define GPIO_HIGH 1		// HIGH出力の定義
#define GPIO_LOW 0		// LOW出力の定義

#define GPIO_LED  25	// GPIO番号の定義

int main(void)
{
    struct gpiod_chip *chip = NULL; // GPIOチップの構造体ポインタ
    struct gpiod_line *line = NULL; // GPIOラインの構造体ポインタ

    // GPIOチップのオープン
    chip = gpiod_chip_open(CHIP_NAME);
    if (!chip) {
        perror("Open chip");
        goto cleanup;
    }

    // GPIOラインの取得
    line = gpiod_chip_get_line(chip, GPIO_LED);
    if (!line) {
        perror("Get line");
        goto cleanup;
    }

    // GPIOラインに出力をリクエスト
    if (gpiod_line_request_output(line, PROGRAM_NAME, GPIO_LOW) < 0) {
        perror("gpiod_line_request_output");
        goto cleanup;
    }

	// LED の点灯、消灯を行う（0.5秒周期）
    for (int i = 0 ; i < 10 ; i++) {
        printf("Turning LED on...\n");

        // GPIOラインの出力を HIGH に設定
        gpiod_line_set_value(line, GPIO_HIGH);

        // 0.5 sec = 500ms = 500000us 待機
        usleep(500000);
        printf("Turning LED off...\n");

        // GPIOラインの出力を LOW に設定
        gpiod_line_set_value(line, GPIO_LOW);
        usleep(500000);
    }

    gpiod_chip_close(chip);
    return 0;

cleanup:
    if (line) gpiod_line_release(line);
    if (chip) gpiod_chip_close(chip);
    return 1;
}


