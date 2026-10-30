// Main program for the light sampler: initializes all modules, waits for
// the UDP "stop" command, then cleans everything up in reverse order.

#include "stop.h"
#include "periodTimer.h"
#include "sampler.h"
#include "reporter.h"
#include "UDP.h"

#include <stdio.h>

int main(void)
{
    printf("Light sampler starting...\n");

    Stop_init();
    Period_init();
    Sampler_init();
    Reporter_init();
    UDP_init();

    Stop_waitForRequest();

    UDP_cleanup();
    Reporter_cleanup();
    Sampler_cleanup();
    Period_cleanup();
    Stop_cleanup();

    printf("Light sampler done.\n");
    return 0;
}
