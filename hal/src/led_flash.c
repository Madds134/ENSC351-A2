#include "hal/led_flash.h"
#include "hal/gpio_init.h"
#include "hal/gpio_write.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define LED_CHIP "/dev/gpiochip2"
#define LED_LINE 42  // GPIO25, header pin 22
#define NUM_LED_LINES 1
#define NO_DEBOUNCE 0

#define NS_PER_SECOND 1000000000L
#define TOGGLES_PER_CYCLE 2  // one flash = on + off

#define LED_OFF 0
#define LED_ON  1

static struct gpiod_line_request *request = NULL;
static pthread_t flashThread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t rateChanged;

// Protected by lock
static int targetHz = 0;
static bool isRunning = false;
static int ledLevel = LED_OFF;

/* Helpers */
static void setLed(int level)
{
    ledLevel = level;
    if (GPIO_Write(request, LED_LINE, level) != 0) {
        fprintf(stderr, "LedFlash: unable to set LED level\n");
    }
}

static void addNs(struct timespec *time, long ns)
{
    time->tv_nsec += ns;
    while (time->tv_nsec >= NS_PER_SECOND) {
        time->tv_sec++;
        time->tv_nsec -= NS_PER_SECOND;
    }
}

/**
 * @brief Turns the LED off and sleeps until the rate changes or the thread stops
 * @warning The caller must hold the lock
 */
static void waitWhileOff(void)
{
    if (ledLevel != LED_OFF) {
        setLed(LED_OFF);
    }
    pthread_cond_wait(&rateChanged, &lock);
}

/* Flashing thread */
/**
 * @brief Toggles the LED on an absolute schedule so timing errors don't drift
 * @details Sleeps on a condition variable, so a rate change wakes it immediately
 */
static void *flashThreadFn(void *arg)
{
    (void)arg;
    struct timespec nextToggle = {0, 0};
    int activeHz = 0;

    pthread_mutex_lock(&lock);
    while (isRunning) {
        if (targetHz <= 0) {
            activeHz = 0;
            waitWhileOff();
            continue;
        }

        long halfPeriodNs = NS_PER_SECOND / (TOGGLES_PER_CYCLE * (long)targetHz);
        if (activeHz != targetHz) {
            // Rate changed: restart the schedule from now
            activeHz = targetHz;
            clock_gettime(CLOCK_MONOTONIC, &nextToggle);
            addNs(&nextToggle, halfPeriodNs);
        }

        int result = pthread_cond_timedwait(&rateChanged, &lock, &nextToggle);
        if (result == ETIMEDOUT && isRunning && targetHz == activeHz) {
            setLed(!ledLevel);
            addNs(&nextToggle, halfPeriodNs);
        }
        // Otherwise woken by a rate change or stop: loop and re-evaluate
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

/* Public functions */
int LedFlash_init(void)
{
    if (request != NULL) {
        fprintf(stderr, "LedFlash_init: already initialized\n");
        return -1;
    }

    unsigned int offsets[NUM_LED_LINES] = {LED_LINE};
    request = GPIO_Init(LED_CHIP, offsets, NUM_LED_LINES,
                        GPIOD_LINE_DIRECTION_OUTPUT, GPIOD_LINE_EDGE_NONE,
                        NO_DEBOUNCE);
    if (request == NULL) {
        return -1;
    }

    // Use the monotonic clock so wall-clock changes don't affect the timing
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&rateChanged, &attr);
    pthread_condattr_destroy(&attr);

    targetHz = 0;
    isRunning = true;
    setLed(LED_OFF);

    if (pthread_create(&flashThread, NULL, flashThreadFn, NULL) != 0) {
        fprintf(stderr, "LedFlash_init: unable to create flashing thread\n");
        isRunning = false;
        pthread_cond_destroy(&rateChanged);
        gpiod_line_request_release(request);
        request = NULL;
        return -1;
    }
    return 0;
}

void LedFlash_cleanup(void)
{
    if (request == NULL) {
        return;
    }

    pthread_mutex_lock(&lock);
    {
        isRunning = false;
        pthread_cond_signal(&rateChanged);
    }
    pthread_mutex_unlock(&lock);
    pthread_join(flashThread, NULL);

    setLed(LED_OFF);
    pthread_cond_destroy(&rateChanged);
    gpiod_line_request_release(request);
    request = NULL;
}

void LedFlash_setHz(int hz)
{
    if (hz < 0) {
        hz = 0;
    }

    pthread_mutex_lock(&lock);
    {
        if (hz != targetHz) {
            targetHz = hz;
            pthread_cond_signal(&rateChanged);
        }
    }
    pthread_mutex_unlock(&lock);
}
