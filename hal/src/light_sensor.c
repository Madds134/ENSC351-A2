#include "hal/light_sensor.h"
#include "hal/adc.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define LIGHT_SENSOR_ADC_CHANNEL 0
#define ADC_REF 3.3

static bool isInitialized = false;

static double raw_to_voltage(int raw)
{
    return ((double)raw / ADC_MAX_READING) * ADC_REF;
}

void LightSensor_init(void) {
    if (isInitialized){
        return;
    }
    ADC_init();
    isInitialized = true;
}

void LightSensor_cleanup(void) {
    if (!isInitialized){
        return;
    }
    ADC_cleanup();
    isInitialized = false;
}

bool LightSensor_readVoltage(double *volts) {
    if(!isInitialized) {
        fprintf(stderr, "LightSensor_init() must be called before LightSensor_readVoltage()\n");
        exit(EXIT_FAILURE);
    }

    int raw = ADC_read_channel(LIGHT_SENSOR_ADC_CHANNEL);
    if (raw == ADC_READ_ERROR) {
        return false;
    }

    *volts = raw_to_voltage(raw);
    return true;
}