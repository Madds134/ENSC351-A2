/**
 * @file led.c
 * @author Madden Wynans
 * @brief Hardware abstraction layer for controlling the BeagleY-AI
 * 
 */
#define _POSIX_C_SOURCE 200809L

#include "hal/led.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Macro Definitions */
#define GREEN_LED_TRIGGER_PATH "/sys/class/leds/ACT/trigger"
#define GREEN_LED_BRIGHTNESS_PATH "/sys/class/leds/ACT/brightness"
#define RED_LED_TRIGGER_PATH "/sys/class/leds/PWR/trigger"
#define RED_LED_BRIGHTNESS_PATH "/sys/class/leds/PWR/brightness"
#define TRIGGER_DELAY 100

static bool isInitialized = false;

/* Function for writing to the Linux Virutal File System 
*  This was derived from the given example on course website here: 
*  https://www.sfu.ca/~mcstewar/ENSC351/ensc351/guides/files_byai/LEDGuide.pdf
*/
static void write_to_file(const char *filePath, const char *value) {
    FILE *file = fopen(filePath, "w");

    if(file == NULL) {
        perror("Error opening LED file");
        exit(EXIT_FAILURE);
    }

    int charsWritten = fprintf(file, "%s", value);

    if (charsWritten <= 0) {
        fclose(file);
        perror("Error writing LED file");
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

static void sleep_ms(long delay_ms) {
    struct timespec delay = {
        .tv_sec = delay_ms / 1000,
        .tv_nsec = (delay_ms % 1000) * 1000000L
    };
    nanosleep(&delay, NULL);
}

void LED_init(void) {

    if(isInitialized) {
        return;
    }

    write_to_file(GREEN_LED_TRIGGER_PATH, "none");
    write_to_file(RED_LED_TRIGGER_PATH, "none");
    sleep_ms(TRIGGER_DELAY);

    isInitialized = true;

    LED_setGreen(false);
    LED_setRed(false);
}

void LED_setGreen(bool on) {
    
    assert(isInitialized);
    write_to_file(GREEN_LED_BRIGHTNESS_PATH, on ? "1" : "0");

}

void LED_setRed(bool on) {
    
    assert(isInitialized);
    write_to_file(RED_LED_BRIGHTNESS_PATH, on ? "1" : "0");
}

void LED_cleanup(void) {
    if(!isInitialized){
        return;
    }

    LED_setGreen(false);
    LED_setRed(false);
    
    isInitialized = false;
}