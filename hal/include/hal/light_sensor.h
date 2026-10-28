/**
 * @file light_sensor.h
 * @author Madden Wynans & Joey Kolsternen
 * @brief HAL for light sensor
 * @date 2026-10-28
 */

 #ifndef LIGHT_SENSOR_H
 #define LIGHT_SENSOR_H

 #include <stdbool.h>

 /**
  * @brief Initializes the light sensor module
  * @warning Must be called before LightSensor_readVoltage()
  */
 void LightSensor_init(void);

 /**
  * @brief Cleans up the light sensor module
  */
 void LightSensor_cleanup(void);

 /**
  * @brief Reads the current light sensor voltage
  * @param volts is the sensor voltage from the ADC
  * @return true on success, false if the ADC read failed
  */
 bool LightSensor_readVoltage(double *volts);

 #endif