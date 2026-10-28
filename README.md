# ENSC 351 Assignment 2: Light Dip Detector

`light_sampler` runs on the BeagleY-AI. It samples a light sensor through the
ADC, counts dips in light level each second, flashes an LED emitter with PWM at
a frequency set by the rotary encoder, and answers commands over UDP port 12345.
