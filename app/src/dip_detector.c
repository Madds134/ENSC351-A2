#include "dip_detector.h"

#include <stdbool.h>

#define DIP_TRIGGER_DROP_VOLTS 0.1
#define DIP_REARM_DROP_VOLTS 0.07

int DipDetector_countDips(const double *samples, int numSamples, double averageVolts)
{
    double triggerLevel = averageVolts - DIP_TRIGGER_DROP_VOLTS;
    double rearmLevel = averageVolts - DIP_REARM_DROP_VOLTS;

    bool isArmed = true;
    int dipCount = 0;
    for(int i = 0; i < numSamples; i++)
    {
        // Voltage level below the threshold
        if(isArmed && samples[i] <= triggerLevel){ 
            dipCount++;
            isArmed = false;
        }
        // Voltage level rises to over 0.07 below average
        else if (!isArmed && samples[i] >= rearmLevel) {
            isArmed = true;
        }
    }
    return dipCount;
}