
#include "hal/adc.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define SPI_BUS "/dev/spidev0.0"
#define SPI_SPEED_HZ 250000
#define SPI_BITS_PER_WORD 8

static int spi_fd = -1; // File Descriptor for SPI (Linux only has positive values for valid file descriptors)
static bool isInitialized = false;

void ADC_init(void) {
    if(isInitialized) {
        return;
    }

    spi_fd = open(SPI_BUS, O_RDWR);
    if (spi_fd < 0) {
        perror("ADC Driver: Unable to open SPI device");
        exit(EXIT_FAILURE);
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = SPI_BITS_PER_WORD;
    uint32_t speed = SPI_SPEED_HZ;

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("ADC Driver: Unable to set SPI mode");
        close(spi_fd);
        exit(EXIT_FAILURE);
    }

    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("ADC Driver: Unable to set SPI bits per word");
        close(spi_fd);
        exit(EXIT_FAILURE);
    }

    if(ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("ADC Driver: Unable to set SPI speed");
        close(spi_fd);
        exit(EXIT_FAILURE);
    }
    
    isInitialized = true;

}

void ADC_cleanup(void) {
    if (!isInitialized) {
        return;
    }
    close(spi_fd);
    spi_fd = -1;
    isInitialized = false;
}

int ADC_read_channel(uint8_t channel) {
    if (!isInitialized) {
        fprintf(stderr, "ADC Driver: ADC_init() must be called before ADC_read_channel()\n");
        exit(EXIT_FAILURE);
    }

    if (channel > 7) {
        fprintf(stderr, "ADC Driver: Invalid Channel %u\n", channel);
        exit(EXIT_FAILURE);
    }

    uint8_t tx[3] =  { (uint8_t)(0x06 | ((channel & 0x04) >> 2)), (uint8_t)((channel & 0x03) << 6), 0x00 };

    uint8_t rx[3] = { 0 };

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = SPI_SPEED_HZ,
        .bits_per_word = SPI_BITS_PER_WORD,
        .cs_change = 0
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("ADC Driver: SPI transfer failed");
        return ADC_READ_ERROR;
    }

    return ((rx[1] & 0x0F) << 8) | rx[2]; // 12-bit result
}
