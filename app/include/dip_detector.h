/**
 * @file dip_detector.h
 * @author Madden Wynans
 * @brief Counts dip in light level within an array of samples
 * @date 2026-10-30
 * 
 * @copyright Copyright (c) 2026
 */
#ifndef DIP_DETECTOR_H
#define DIP_DETECTOR_H

/**
 * @brief Counts the light dips in a set of samples
 * 
 * @param samples Sample voltages
 * @param numSamples Number of samples
 * @param averageVolts Average light level 
 * @return int Number of dips found
 */
int DipDetector_countDips(const double *samples, int numSamples, double averageVolts);
#endif


