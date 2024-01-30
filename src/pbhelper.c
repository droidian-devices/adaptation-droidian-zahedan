/*
 * Copyright (C) 2024 Bardia Moshiri
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Bardia Moshiri <fakeshell@bardia.tech>
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <sys/select.h>
#include <linux/input.h>
#include <batman/wlrdisplay.h>

// Convert seconds to micros
#define WRITE_SLEEP_PERIOD 0.1*1000000

// For debug uncomment
// #define DEBUG
#define READ_FORMAT "Read: %d bytes from %s\n"
#define WRITE_FORMAT "Wrote: %d bytes to %s\n"

#define DEVICE_FILE "/dev/input/event1"

void debug(const char* format, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}

int main(int argc, char* argv[]) {
    setenv("XDG_RUNTIME_DIR", "/run/user/32011", 1);

    int brightness_control;
    char before_lock[4];
    int bwritten;
    int bread_before;
    bool first_run = true;
    bool screen_on = true;
    bool screen_interrupted = false;
    bool screen_interrupted_old = true;

    struct input_event ev;
    int fd_device = open(DEVICE_FILE, O_RDONLY);
    if (fd_device == -1) {
        perror("Cannot access input device");
        exit(EXIT_FAILURE);
    }

    // Set fd_device to non-blocking mode
    int flags = fcntl(fd_device, F_GETFL, 0);
    fcntl(fd_device, F_SETFL, flags | O_NONBLOCK);

    fd_set readfds;
    struct timeval tv;
    int ret;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd_device, &readfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(fd_device + 1, &readfds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(fd_device, &readfds)) {
            if (read(fd_device, &ev, sizeof(struct input_event)) > 0) {
                if (ev.type == EV_KEY && ev.value == 1 && ev.code == 116) { // Power button pressed
                    screen_on = wlrdisplay(argc, argv);
                    debug("Screen state changed: %s\n", screen_on ? "On" : "Off");

                    if (screen_on) {
                        if (!first_run) {
                            debug("Setting Brightness to 0\n");

                            brightness_control = open("/sys/class/leds/lcd-backlight/brightness", O_WRONLY);
                            bwritten = write(brightness_control, "0", 1);
                            close(brightness_control);
                            debug(WRITE_FORMAT, bwritten, "/sys/class/leds/lcd-backlight/brightness");

                            usleep(WRITE_SLEEP_PERIOD);

                            debug("Setting Brightness to %s\n", before_lock);
                            brightness_control = open("/sys/class/leds/lcd-backlight/brightness", O_WRONLY);
                            bwritten = write(brightness_control, before_lock, strlen(before_lock));
                            close(brightness_control);

                            debug(WRITE_FORMAT, bwritten, "/sys/class/leds/lcd-backlight/brightness");
                        }
                    } else {
                        memset(before_lock, 0, sizeof(before_lock));
                        brightness_control = open("/sys/class/leds/lcd-backlight/brightness", O_RDONLY);
                        bread_before = read(brightness_control, before_lock, sizeof(before_lock) - 1);
                        close(brightness_control);

                        debug(READ_FORMAT, bread_before, "/sys/class/leds/lcd-backlight/brightness");
                        debug("Current Brightness: %s\n", before_lock);

                        first_run = false;
                    }
                }
            }
        }

        screen_interrupted = !wlrdisplay(argc, argv);
        debug("screen_interrupted: %d screen_interrupted_old: %d, screen_on: %d\n", screen_interrupted, screen_interrupted_old, screen_on);
        if (screen_interrupted != screen_interrupted_old) {
            debug("Screen state changed: %s\n", screen_interrupted ? "On" : "Off");

            if (screen_interrupted && screen_on == 0) {
                debug("Setting Brightness to 0\n");

                brightness_control = open("/sys/class/leds/lcd-backlight/brightness", O_WRONLY);
                bwritten = write(brightness_control, "0", 1);
                close(brightness_control);
                debug(WRITE_FORMAT, bwritten, "/sys/class/leds/lcd-backlight/brightness");

                usleep(WRITE_SLEEP_PERIOD);

                debug("Setting Brightness to %s\n", before_lock);
                brightness_control = open("/sys/class/leds/lcd-backlight/brightness", O_WRONLY);
                bwritten = write(brightness_control, before_lock, strlen(before_lock));
                close(brightness_control);

                debug(WRITE_FORMAT, bwritten, "/sys/class/leds/lcd-backlight/brightness");
            }
            screen_interrupted_old = screen_interrupted;
        }
    }

    close(fd_device);
    return 0;
}
