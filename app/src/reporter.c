#include "reporter.h"
#include "sampler.h"
#include "periodTimer.h"
#include "hal/timing.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPORT_INTERVAL_MS 1000
#define NUM_SAMPLES_TO_REPORT 10

// test value
#define TEST_FLASH_HZ 0

static int dipCount = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_t reporterThread;
static atomic_bool isStopping = false;
static bool isInitialized = false;

/* Helpers */
static void setDipCount(int count)
{
    pthread_mutex_lock(&lock);
    {
        dipCount = count;
    }
    pthread_mutex_unlock(&lock);
}

static void print_stats(int numSamples, double avg, int dips, const Period_statistics_t *stats)
{
        printf("#Smpl/s = %4d  Flash @ %3dHz  avg = %5.3fV  dips = %3d  Smpl ms[%6.3f, %6.3f] avg %6.3f/%4d\n",
           numSamples, TEST_FLASH_HZ, avg, dips,
           stats->minPeriodInMs, stats->maxPeriodInMs, stats->avgPeriodInMs, stats->numSamples);
}

static void print_sample_line(const double *samples, int numSamples)
{
    int numToPrint = (numSamples < NUM_SAMPLES_TO_REPORT) ? numSamples : NUM_SAMPLES_TO_REPORT;
    for(int i = 0; i < numToPrint; i++)
    {
        int index = (numSamples > NUM_SAMPLES_TO_REPORT) ? i * numSamples / NUM_SAMPLES_TO_REPORT : i;
        printf("%4d:%5.3f", index, samples[index]);
    }
    printf("\n");
}

static void report_one_second(void)
{
    Sampler_moveCurrentDataToHistory();

    Period_statistics_t stats;
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

    int numSamples;
    double *samples = Sampler_getHistory(&numSamples);
    double avg = Sampler_getAverageReading();

    int dips = 0;
    setDipCount(dips);

    print_stats(numSamples, avg, dips, &stats);
    print_sample_line(samples, numSamples);

    free(samples);
}

/* Reporter thread */
static void *reporterThreadFn(void *arg)
{
    (void)arg;
    while (!atomic_load(&isStopping)) {
        Timing_sleepMs(REPORT_INTERVAL_MS);
        report_one_second();
    }
    return NULL;
}

/* Init/cleanup */
void Reporter_init(void)
{
    if(isInitialized) {
        return;
    }
    atomic_store(&isStopping, false);
    int err = pthread_create(&reporterThread, NULL, reporterThreadFn, NULL);
    if (err != 0) {
        fprintf(stderr, "Unable to create reporter thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
}

void Reporter_cleanup(void)
{
    if(!isInitialized){
        return;
    }
    atomic_store(&isStopping, true);
    pthread_join(reporterThread, NULL);
    isInitialized = false;
}

int Reporter_getDipCount(void)
{
    int count;
    pthread_mutex_lock(&lock);
    {
        count = dipCount;
    }
    pthread_mutex_unlock(&lock);
    return count;
}