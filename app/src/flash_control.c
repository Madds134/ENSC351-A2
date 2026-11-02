#include "flash_control.h"
#include "hal/rotary_encoder.h"
#include "hal/led_flash.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_HZ 10
#define MIN_HZ   0
#define MAX_HZ   500

// How long to wait for an encoder step before re-checking for shutdown
#define STEP_TIMEOUT_MS 100

static int currentHz = START_HZ;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_t encoderThread;
static atomic_bool isStopping = false;
static bool isInitialized = false;

/* Helpers */
static int clampHz(int hz)
{
    if (hz < MIN_HZ) {
        return MIN_HZ;
    }
    if (hz > MAX_HZ) {
        return MAX_HZ;
    }
    return hz;
}

/**
 * @brief Applies one encoder step to the flash rate
 * @details Only updates the LED if the rate actually changed, so turning
 *          past a limit does not interrupt the flashing
 */
static void applyStep(int step)
{
    pthread_mutex_lock(&lock);
    {
        int newHz = clampHz(currentHz + step);
        if (newHz != currentHz) {
            currentHz = newHz;
            LedFlash_setHz(currentHz);
        }
    }
    pthread_mutex_unlock(&lock);
}

/* Thread */
static void *encoderThreadFn(void *arg)
{
    (void)arg;
    while (!atomic_load(&isStopping)) {
        int step = 0;
        int ret = Rotary_waitStep(STEP_TIMEOUT_MS, &step);
        if (ret < 0) {
            fprintf(stderr, "Rotary encoder read failed; encoder disabled\n");
            break;
        }
        if (ret == 1) {
            applyStep(step);
        }
    }
    return NULL;
}

/* Init/cleanup */
void FlashControl_init(void)
{
    if (isInitialized) {
        return;
    }

    if (Rotary_init() != 0) {
        fprintf(stderr, "Unable to initialize rotary encoder\n");
        exit(EXIT_FAILURE);
    }
    if (LedFlash_init() != 0) {
        fprintf(stderr, "Unable to initialize flashing LED\n");
        Rotary_cleanup();
        exit(EXIT_FAILURE);
    }

    currentHz = START_HZ;
    LedFlash_setHz(currentHz);

    atomic_store(&isStopping, false);
    int err = pthread_create(&encoderThread, NULL, encoderThreadFn, NULL);
    if (err != 0) {
        fprintf(stderr, "Unable to create encoder thread: %s\n", strerror(err));
        LedFlash_cleanup();
        Rotary_cleanup();
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
}

void FlashControl_cleanup(void)
{
    if (!isInitialized) {
        return;
    }
    atomic_store(&isStopping, true);
    pthread_join(encoderThread, NULL);

    LedFlash_cleanup();
    Rotary_cleanup();
    isInitialized = false;
}

int FlashControl_getHz(void)
{
    int hz;
    pthread_mutex_lock(&lock);
    {
        hz = currentHz;
    }
    pthread_mutex_unlock(&lock);
    return hz;
}
