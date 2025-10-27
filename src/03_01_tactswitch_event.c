#include <gpiod.h>
#include <pthread.h> // イベントハンドラ "スレッド" 用
#include <poll.h>    // ポーリングライブラリ
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> // 割り込みハンドラの定義ファイル

#define GPIO_CHIP "/dev/gpiochip0"
#define GPIO_LINE 27

volatile int loop = 1; // ループ制御用フラグ
void sigint_handler(__attribute__((unused)) int signum)
{
    loop = 0; // ループを終了するためのフラグをセット
}

typedef void (*gpio_event_handler_t)(int gpio, int event_type);

typedef struct {
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int fd;
    gpio_event_handler_t handler;
    pthread_t thread;
    int stop;
} gpio_monitor_t;

void* gpio_thread_func(void *arg) {
    gpio_monitor_t *monitor = (gpio_monitor_t*)arg;
    struct pollfd pfd = { .fd = monitor->fd, .events = POLLIN };
    struct gpiod_line_event event;

    while (!monitor->stop) {
        int ret = poll(&pfd, 1, 1000); // 1秒おきにタイムアウトチェック
        if (ret < 0) {
            perror("poll");
            break;
        } else if (ret == 0) {
            continue; // timeout
        }

        if (pfd.revents & POLLIN) {
            if (gpiod_line_event_read(monitor->line, &event) == 0) {
                if (monitor->handler) {
                    monitor->handler(GPIO_LINE, event.event_type);
                }
            }
        }
    }

    return NULL;
}

int gpio_monitor_start(gpio_monitor_t *monitor, gpio_event_handler_t handler) {
    memset(monitor, 0, sizeof(gpio_monitor_t));
    monitor->handler = handler;

    monitor->chip = gpiod_chip_open(GPIO_CHIP);
    if (!monitor->chip) return -1;

    monitor->line = gpiod_chip_get_line(monitor->chip, GPIO_LINE);
    if (!monitor->line) return -1;

    if (gpiod_line_request_both_edges_events(monitor->line, "gpio-dispatch")) return -1;
    monitor->fd = gpiod_line_event_get_fd(monitor->line);

    return pthread_create(&monitor->thread, NULL, gpio_thread_func, monitor);
}

void gpio_monitor_stop(gpio_monitor_t *monitor) {
    monitor->stop = 1;
    pthread_join(monitor->thread, NULL);
    gpiod_line_release(monitor->line);
    gpiod_chip_close(monitor->chip);
}

void on_gpio_event(int gpio, int event_type) {
    if (event_type == GPIOD_LINE_EVENT_RISING_EDGE)
        printf("GPIO %d: Rising edge\n", gpio);
    else if (event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
        printf("GPIO %d: Falling edge\n", gpio);
}

int main() {
    // Ctrl+Cで終了できるようにシグナルハンドラを設定
    if ( signal(SIGINT, sigint_handler) == SIG_ERR ) {
        perror("signal");
        exit(100);
    }

    gpio_monitor_t monitor;
    if (gpio_monitor_start(&monitor, on_gpio_event) != 0) {
        fprintf(stderr, "Failed to start GPIO monitor\n");
        return 1;
    }

    printf("Monitoring GPIO... Press Ctrl+C to stop.\n");
    while (loop) pause(); // 実際は SIGINT を使って停止処理を入れるのが良い

    gpio_monitor_stop(&monitor);
    return 0;
}
