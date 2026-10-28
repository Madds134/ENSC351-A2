// Main program for the light sampler: initializes all modules, waits for
// the shutdown request, then cleans everything up.
// TEMPORARY: runs for a fixed time until UDP "stop" exists (step 6).

#include "sampler.h"
#include "periodTimer.h"
#include "reporter.h"
#include "hal/timing.h"

#include <stdio.h>

#define TEST_RUN_TIME_MS 10000

int main(void)
{
    printf("Light sampler starting...\n");

    Period_init();
    Sampler_init();
    Reporter_init();

    Timing_sleepMs(TEST_RUN_TIME_MS);

    Reporter_cleanup();
    Sampler_cleanup();
    Period_cleanup();

    printf("Light sampler done.\n");
    return 0;
}
