
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

// MCP3208 command format (3-byte transfer):
//   byte 0: 0 0 0 0 0 START SGL D2
//   byte 1: D1 D0 x x x x x x
//   reply:  result bits B11..B8 in the low nibble of byte 1, B7..B0 in byte 2
#define ADC_TRANSFER_LENGTH 3
#define ADC_MAX_CHANNEL 7
#define ADC_START_BIT 0x04
#define ADC_SINGLE_ENDED_BIT 0x02
#define ADC_CHANNEL_HIGH_BIT 0x04   // D2, sent in byte 0
#define ADC_CHANNEL_HIGH_SHIFT 2
#define ADC_CHANNEL_LOW_BITS 0x03   // D1 D0, sent at the top of byte 1
#define ADC_CHANNEL_LOW_SHIFT 6
#define ADC_RESULT_HIGH_MASK 0x0F
#define BITS_PER_BYTE 8

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

    if (channel > ADC_MAX_CHANNEL) {
        fprintf(stderr, "ADC Driver: Invalid Channel %u\n", channel);
        exit(EXIT_FAILURE);
    }

    uint8_t tx[ADC_TRANSFER_LENGTH] = {
        (uint8_t)(ADC_START_BIT | ADC_SINGLE_ENDED_BIT |
                  ((channel & ADC_CHANNEL_HIGH_BIT) >> ADC_CHANNEL_HIGH_SHIFT)),
        (uint8_t)((channel & ADC_CHANNEL_LOW_BITS) << ADC_CHANNEL_LOW_SHIFT),
        0x00
    };
    uint8_t rx[ADC_TRANSFER_LENGTH] = { 0 };

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ADC_TRANSFER_LENGTH,
        .speed_hz = SPI_SPEED_HZ,
        .bits_per_word = SPI_BITS_PER_WORD,
        .cs_change = 0
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("ADC Driver: SPI transfer failed");
        return ADC_READ_ERROR;
    }

    return ((rx[1] & ADC_RESULT_HIGH_MASK) << BITS_PER_BYTE) | rx[2]; // 12-bit result
}
