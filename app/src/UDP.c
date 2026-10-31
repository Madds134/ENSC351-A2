#include "UDP.h"
#include "stop.h"
#include "sampler.h"
#include "reporter.h"
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

#define UDP_PORT 12345
#define DEFAULT_PROTOCOL 0
#define MAX_PACKET_SIZE 1500
#define NO_FLAGS 0
#define REPLY_BUFFER_SIZE 256
#define WORD_SEPARATORS " \t\r\n"
#define HELP_OUTPUT \
    "\nAccepted command examples:\n" \
    "count       -- get the total number of samples taken.\n" \
    "length      -- get the number of samples taken in the previously completed second.\n" \
    "dips        -- get the number of dips in the previously completed second.\n" \
    "history     -- get all the samples in the previously completed second.\n" \
    "stop        -- cause the server program to end.\n" \
    "<enter>     -- repeat last command.\n"
#define UNKNOWN_OUTPUT "Unknown command. Type 'help' for the list of commands.\n"
#define VALUES_PER_LINE 10
#define SAMPLE_TEXT_SIZE 20
#define NO_HISTORY_OUTPUT "No samples in the history.\n"

static int socketFd = -1;
static pthread_t udpThread;
static bool isInitialized = false;

/**
 * @brief Creates the UDP socket and binds it to UDP_PORT
 * @note Claude and lecture slices were used to help with syntax needed to open socket
 */
static void openSocket(void)
{
    socketFd = socket(AF_INET, SOCK_DGRAM, DEFAULT_PROTOCOL);
    if (socketFd < 0) {
        perror("Unable to create UDP socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(UDP_PORT);

    if (bind(socketFd, (struct sockaddr *)&address, sizeof(address)) != 0) {
        perror("Unable to bind UDP port");
        close(socketFd);
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Sends a '\0' terminated text reply back to client
 * 
 * @param message text to send
 * @param sender address of the client that sent the command
 */
static void sendReply(const char *message, const struct sockaddr_in *sender)
{
    sendto(socketFd, message, strlen(message), NO_FLAGS, 
            (const struct sockaddr *)sender, sizeof(*sender));
}

/* Command handlers for replies */
/**
 * @brief Function signature shared by every UDP command handler
 * @details Each handler builds a specific reply to send
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running, false to terminate it
 */
typedef bool (*CommandHandler)(const struct sockaddr_in *sender); // Function pointer to specific command handler

/**
 * @brief Pair command with the handler
 */
typedef struct {
    const char *name;
    CommandHandler handler;
} Command;

/**
 * @brief Replies with the reply text defined in the help text
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running
 */
static bool handleHelpCommand(const struct sockaddr_in *sender)
{
    sendReply(HELP_OUTPUT, sender);
    return true;
}

/**
 * @brief Replies with the total number of samples taken
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running
 */
static bool handleCountCommand(const struct sockaddr_in *sender)
{
    char reply[REPLY_BUFFER_SIZE];
    snprintf(reply, sizeof(reply), "# samples taken total: %lld\n", Sampler_getNumSamplesTaken());
    sendReply(reply, sender);
    return true;
}

/**
 * @brief Replies with the number of samples in previous second
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running 
 */
static bool handleLengthCommand(const struct sockaddr_in *sender)
{
    char reply[REPLY_BUFFER_SIZE];
    snprintf(reply, sizeof(reply), "# samples taken last second: %d\n", Sampler_getHistorySize());
    sendReply(reply, sender);
    return true;
}

/**
 * @brief Replies with number of dips found in previous second
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running 
 */
static bool handleDipsCommand(const struct sockaddr_in *sender)
{
    char reply[REPLY_BUFFER_SIZE];
    snprintf(reply, sizeof(reply), "# Dips: %d\n", Reporter_getDipCount());
    sendReply(reply, sender);
    return true;
}

/**
 * @brief Repies "Program terminating." and requests program to stop
 * @param sender address of the client that sent the command
 * @return false to end receive loop
 */
static bool handleStopCommand(const struct sockaddr_in *sender)
{
    sendReply("Program terminating.\n", sender);
    Stop_request();
    return false;
}

/**
 * @brief Sends bytes as one packet
 * 
 * @note Claude was used to assist in the syntax needed to send a packet back to the client
 * @param packet Buffer holding the packet bytes
 * @param packetLength Bytes in the packet (Set to zero after sending)
 * @param sender address of the client that sent the command
 */
static void sendPacket(const char *packet, size_t *packetLength, const struct sockaddr_in *sender)
{
    if (*packetLength > 0) {
        sendto(socketFd, packet, *packetLength, NO_FLAGS,
               (const struct sockaddr *)sender, sizeof(*sender));
        *packetLength = 0;
    }
}

/**
 * @brief Formats one sample for the history reply
 * 
 * @param text Output buffer for the formatted text
 * @param textSize Output buffer size
 * @param volts Sample voltage
 * @param index Index of sample in history
 * @param numSamples Total smaples in history
 * @return size_t Number of characters written
 */
static size_t formatSample(char *text, size_t textSize, double volts, int index, int numSamples)
{
    bool isEndOfLine = ((index + 1) % VALUES_PER_LINE == 0) || (index + 1 == numSamples);
    int length = snprintf(text, textSize, "%.3f%s", volts, isEndOfLine ? ",\n" : ", ");
    return (size_t)length;
}

/**
 * @brief Replies with dump of every sample from previous second
 * @param sender address of the client that sent the command
 * @return true to keep UDP server running 
 */
static bool handleHistoryCommand(const struct sockaddr_in *sender)
{
    int numSamples;
    double *samples = Sampler_getHistory(&numSamples);
    if (numSamples == 0) {
        sendReply(NO_HISTORY_OUTPUT, sender);
        return true;
    }

    char packet[MAX_PACKET_SIZE];
    size_t packetLength = 0;
    for (int i = 0; i < numSamples; i++) {
        char text[SAMPLE_TEXT_SIZE];
        size_t textLength = formatSample(text, sizeof(text), samples[i], i, numSamples);

        if (packetLength + textLength > MAX_PACKET_SIZE) {
            sendPacket(packet, &packetLength, sender);
        }
        memcpy(packet + packetLength, text, textLength); // Append formatted value to packet
        packetLength += textLength;
    }
    sendPacket(packet, &packetLength, sender);

    free(samples);
    return true;
}

/* Command LUT */
static const Command Commands[] = {
    {"help", handleHelpCommand},
    {"?", handleHelpCommand},
    {"count", handleCountCommand},
    {"length", handleLengthCommand},
    {"dips", handleDipsCommand},
    {"history", handleHistoryCommand},
    {"stop", handleStopCommand},
};

#define NUM_COMMANDS (sizeof(Commands) / sizeof(Commands[0]))

static const Command *lastCommand = NULL;

/**
 * @brief Looks up command in LUT for first word of packet
 * 
 * @param word Start of the received command
 * @param wordLength Number of characters in first word
 * @note Claude was used to explain how the strncasecmp() function works
 * @return const Command* Pointer to macthing LUT entry, or NULL if unknown
 */
static const Command *findCommand(const char *word, size_t wordLength)
{
    for(size_t i = 0; i < NUM_COMMANDS; i++)
    {
        const char *name = Commands[i].name;

        // Check if word is correct
        bool sameLength = (strlen(name) == wordLength);

        // Case insentitivity for first word of command
        bool sameCharacters = (strncasecmp(word, name, wordLength) == 0);

        if (sameLength && sameCharacters) {
            return &Commands[i]; // Return row in LUT
        }
    }
    return NULL;
}

/**
 * @brief Runs the command in one received packet and replies to the sender
 * 
 * @param command Terminated packet text
 * @param sender address of the client that sent the command
 * @return false if server should stip, true otherwise
 */
static bool handleIncomingCommand(const char *command, const struct sockaddr_in *sender)
{
    size_t wordLength = strcspn(command, WORD_SEPARATORS);
    const Command *toRun = (wordLength == 0) ? lastCommand : findCommand(command, wordLength);

    lastCommand = toRun;
    if (toRun == NULL) {
        sendReply(UNKNOWN_OUTPUT, sender);
        return true;
    }
    return toRun->handler(sender);
}

/**
 * @brief UDP server thread, receives one command per packet and replies
 * 
 * @param arg Unsued
 * @return void* 
 */
static void *udpThreadFn(void *arg)
{
    (void)arg;
    char command[MAX_PACKET_SIZE + 1];
    bool keepRunning = true;

    while(keepRunning) {
        struct sockaddr_in sender;
        socklen_t senderLength = sizeof(sender);
        ssize_t numBytes = recvfrom(socketFd, command, MAX_PACKET_SIZE, NO_FLAGS,
                                    (struct sockaddr *)&sender, &senderLength);
        
        if (numBytes < 0) {
            perror("UDP receive failed");
            continue;
        }
        command[numBytes] = '\0';
        keepRunning = handleIncomingCommand(command, &sender);
    }
    return NULL;
}

void UDP_init(void)
{
    if (isInitialized) {
        return;
    }
    openSocket();
    int err = pthread_create(&udpThread, NULL, udpThreadFn, NULL);
    if (err != 0) {
        fprintf(stderr, "Unable to create UDP thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
}

void UDP_cleanup(void)
{
    if(!isInitialized) {
        return;
    }
    pthread_join(udpThread, NULL);
    close(socketFd);
    socketFd = -1;
    isInitialized = false;

}