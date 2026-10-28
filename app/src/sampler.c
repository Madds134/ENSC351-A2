#include "sampler.h"
#include "periodTimer.h"
#include "hal/light_sensor.h"
#include "hal/timing.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAMPLES_PER_SECOND 4096
#define SAMPLE_INTERVAL_MS 1
#define AVG_WEIGHT_PREV 0.999
#define AVG_WEIGHT_NEW (1.0 - AVG_WEIGHT_PREV)

// Ping-pong buffers: the thread fills current, readers copy from history
static double bufferA[MAX_SAMPLES_PER_SECOND];
static double bufferB[MAX_SAMPLES_PER_SECOND];
static double *current = bufferA;
static double *history = bufferB;
static int currentSize = 0;
static int historySize = 0;

static double averageReading = 0.0;
static bool hasFirstSample = false; // Average starts at real value
static long long numSamples = 0;

// Mutex to protect the shared buffers
static pthread_mutex_t lock= PTHREAD_MUTEX_INITIALIZER;

static pthread_t samplerThread;
static atomic_bool isStopping = false; // Main thread tells sampler thread to finish
static bool isInitialized = false;

/* Sampler thread */
/**
 * @brief Updates the exponeentially smoothed average with new sample
 * 
 * @param volts the newest sample voltage
 * @warning The caller must hold the mutex
 */
static void update_average(double volts)
{
    if(!hasFirstSample) {
        averageReading = volts; // first sample starts the average
        hasFirstSample = true;
    } 
    else {
        averageReading = AVG_WEIGHT_PREV * averageReading + AVG_WEIGHT_NEW * volts;
    }
}

/**
 * @brief recrods one sample, updates the average, total counts and appends it to the current bugger
 * 
 * @details Locks the mutex for the whole update
 * @param volts The sample voltage to record
 * @return true if the sample was stored
 * @return false if the current buffer is full and the sample was dropped
 */
static bool recordSample(double volts) 
{
    bool stored = false;
    pthread_mutex_lock(&lock);
    {
        update_average(volts);
        numSamples++;
        if (currentSize < MAX_SAMPLES_PER_SECOND) {
            current[currentSize] = volts;
            currentSize++;
            stored = true;        
        }
    }
    pthread_mutex_unlock(&lock);
    return stored;
}

/** 
 * @brief Sampling thread: reads the light sensor once per ms
 * 
 * @details Skips failed ADC reads, marks PERIOD_EVENT_SAMPLE_LIGHT for each recorded sample
 * @param arg Is not used is just required to create the trhead
 * @return void* 
 * 
 * @note Claude was used to help with Thread syntax of void *fn(void *arg)
 *       
 */
static void *samplerThreadFn(void *arg)
{
    (void)arg;
    while (!atomic_load(&isStopping)) {
        double volts;
        if(LightSensor_readVoltage(&volts)) {
            if (!recordSample(volts)) {
                fprintf(stderr, "Sampler current buffer is full, sample is dropped\n");
            }
            Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        }
        Timing_sleepMs(SAMPLE_INTERVAL_MS);
    }
    return NULL;
}

/* Init/Cleanup */
void Sampler_init(void)
{
    if (isInitialized) {
        return;
    }
    LightSensor_init();

    atomic_store(&isStopping, false);
    int err = pthread_create(&samplerThread, NULL, samplerThreadFn, NULL);
    if (err != 0) {
        fprintf(stderr, "Sampler thread was unable to be created: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
}

void Sampler_cleanup(void)
{
    if(!isInitialized) {
        return;
    }
    atomic_store(&isStopping, true);
    pthread_join(samplerThread, NULL);

    LightSensor_cleanup();
    isInitialized = false;
}

/* Pulic accessors */
/**
 * @brief Moves the samples collected this second into the history buffer
 * 
 * @details Swaps the current and history buffers, so reader can read data 
 * while thread fills new buffer
 * @warning Must be called every 1s
 */
void Sampler_moveCurrentDataToHistory(void)
{
    pthread_mutex_lock(&lock);
    {
        double *temp = history;
        history = current;
        current = temp;

        historySize = currentSize;
        currentSize = 0;
    }
    pthread_mutex_unlock(&lock);
}

/**
 * @brief Gets the number of samples collected during the previous second
 * 
 * @return int The number of samples in the history buffer
 */
int Sampler_getHistorySize(void)
{
    int size;
    pthread_mutex_lock(&lock);
    {
        size = historySize;
    }
    pthread_mutex_unlock(&lock);
    return size;
}

/**
 * @brief Gets a copy of the samples in history
 * 
 * @details Returns a newly allocated array, so it persists for the caller
 * @param size Output parameter set to the number of elements in the returned array
 *             Is 0 if history buffer is empty or allocation failed
 * @return double* array of sample voltages, or NULL if size is 0
 * @warning THE CALLER MUST FREE RETURNED POINTER
 * 
 * @note Claude was used to help with allocating memory for the return array.
 *       High level logic is from me. Claude assisted in implemented the code to allocate the array.
 */
double* Sampler_getHistory(int *size)
{
    double *copy = NULL;
    pthread_mutex_lock(&lock);
    {
        *size = 0;
        if (historySize > 0) {
            copy = malloc(historySize * sizeof(*copy));
            if (copy != NULL) {
                memcpy(copy, history, historySize * sizeof(*copy));
                *size = historySize;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    return copy;
}

/**
 * @brief Get the exponentially smoothed light level
 * 
 * @return double Average sensor voltage, weighting the previous average at 99.9%
 */
double Sampler_getAverageReading(void)
{
    double avg;
    pthread_mutex_lock(&lock);
    {
        avg = averageReading;
    }
    pthread_mutex_unlock(&lock);
    return avg;
}

/**
 * @brief Gets the total number of light samples 
 * 
 * @return long long Total sample count, including dropped ones
 */
long long Sampler_getNumSamplesTaken(void)
{
    long long count;
    pthread_mutex_lock(&lock);
    {
        count = numSamples;
    }
    pthread_mutex_unlock(&lock);
    return count;
}


