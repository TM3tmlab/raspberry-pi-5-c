#include <gpiod.h>	// gpiodの定義ファイル
#include <unistd.h>	// sleep関数の定義ファイル
#include <stdio.h>  // printf関数の定義ファイル
#include <stdlib.h> // exit関数の定義ファイル
#include <signal.h> // 割り込みハンドラの定義ファイル

#define PROGRAM_NAME "takt-sw"	// プログラム名の定義
#define CHIP_NAME "/dev/gpiochip0"    // GPIOチップの定義

#define GPIO_HIGH 1		// HIGH出力の定義
#define GPIO_LOW 0		// LOW出力の定義

#define GPIO_TACT  27	// GPIO番号の定義

// 無限ループがあるので、SIGINT（Ctrl+C）で終了できるようにする
volatile int loop = 1; // ループ制御用フラグ
void sigint_handler(__attribute__((unused)) int signum)
{
    loop = 0; // ループを終了するためのフラグをセット
}

/*
 * GPIOラインのイベントを待機して、押下イベントを処理する
 *
 * Raspberry Pi 5 の場合、入力ラインの待機状態が HIGH のため、
 * プルアップする回路を接続しておく必要がある
 * 
 * プルアップのため、押下イベントは FALLING_EDGE で、離したイベントは RISING_EDGE となる
 */
int main(void)
{
    struct gpiod_chip *chip = NULL; // GPIOチップの構造体ポインタ
    struct gpiod_line *line_tact = NULL; // GPIOラインの構造体ポインタ
    struct gpiod_line_event event; // GPIOラインイベントの構造体

    // Ctrl+Cで終了できるようにシグナルハンドラを設定
    if ( signal(SIGINT, sigint_handler) == SIG_ERR ) {
        perror("signal");
        exit(100);
    }

    // GPIOチップのオープン
    chip = gpiod_chip_open(CHIP_NAME);
    if (!chip) {
        perror("Open chip");
        goto cleanup;
    }

    // GPIOラインの取得
    line_tact = gpiod_chip_get_line(chip, GPIO_TACT);
    if (!line_tact) {
        perror("Get line");
        goto cleanup;
    }

    // 入力 & エッジ検出を要求（falling edge = ボタン押下）
    if (gpiod_line_request_both_edges_events(line_tact, "button-event") < 0) {
        perror("Request event");
        goto cleanup;
    }

    printf("Waiting for button press on GPIO %d...\n", GPIO_TACT);

    int counter = 0;
    while (loop) {
        int ret = gpiod_line_event_wait(line_tact, NULL); // タイムアウトなしで待機
        if (ret < 0) {
            perror("Wait event");
            break;
        }

        // イベントの読み取り
        if (gpiod_line_event_read(line_tact, &event) < 0) {
            perror("Read event");
            break;
        }

        // ボタン押下イベントの処理
        // 初期状態が HIGH なので 押下(FALLING) の際は LED を点灯
        // 離した(RISING) の際は LED を消灯
        if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
            printf("Button pressed! [%d]\n", counter++);
        } else if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            printf("Button released! [%d]\n", counter++);
        }
    }

    gpiod_line_release(line_tact);
    gpiod_chip_close(chip);
    return 0;

cleanup:
    if (line_tact) gpiod_line_release(line_tact);
    if (chip) gpiod_chip_close(chip);
    return 1;
}


