// Main program for the light sampler: initializes all modules, waits for
// the shutdown request, then cleans everything up.
// TEMPORARY: step 3 test loop that prints sampler stats each second.

#include "sampler.h"
#include "periodTimer.h"
#include "hal/timing.h"

#include <stdio.h>
#include <stdlib.h>

#define TEST_NUM_SECONDS 5
#define MS_PER_SECOND 1000

static void printSecondStats(void)
{
    Period_statistics_t stats;
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

    int size;
    double *samples = Sampler_getHistory(&size);
    double first = (size > 0) ? samples[0] : 0.0;

    printf("#Smpl/s = %4d  avg = %.3fV  total = %lld  Smpl ms[%6.3f, %6.3f] avg %6.3f/%d  first = %.3fV\n",
           size, Sampler_getAverageReading(), Sampler_getNumSamplesTaken(),
           stats.minPeriodInMs, stats.maxPeriodInMs, stats.avgPeriodInMs, stats.numSamples,
           first);
    free(samples);
}

int main(void)
{
    printf("Light sampler starting...\n");

    Period_init();
    Sampler_init();

    for (int i = 0; i < TEST_NUM_SECONDS; i++) {
        Timing_sleepMs(MS_PER_SECOND);
        Sampler_moveCurrentDataToHistory();
        printSecondStats();
    }

    Sampler_cleanup();
    Period_cleanup();

    printf("Light sampler done.\n");
    return 0;
}
