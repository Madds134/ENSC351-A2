#include "hal/rotary_encoder.h"
#include "hal/gpio_init.h"
#include "hal/gpio_read.h"

#include <gpiod.h>
#include <stdbool.h>
#include <stdio.h>

#define ROTARY_CHIP   "/dev/gpiochip1"
#define ROTARY_LINE_A 7   // GPIO23, header pin 16
#define ROTARY_LINE_B 10  // GPIO24, header pin 18
#define NUM_ROTARY_LINES 2
#define LINE_A_INDEX 0
#define LINE_B_INDEX 1
#define ROTARY_DEBOUNCE_US 0  // kernel debounce drops real edges on this board; must stay 0
#define EVENT_BATCH_SIZE 64    // edge events drained from the kernel per wake-up

/*
 * Reading the pins
 *
 * Edge events only tell this module *that* the knob moved. Under load the
 * kernel can merge quick edges and deliver A/B events out of order, which
 * scrambles the direction if the events are replayed. So on every wake-up the
 * queued events are discarded and both pins are read together; that snapshot
 * of the real levels is what the decoder sees.
 *
 * Quadrature decoder
 *
 * Pin state code = (A << 1) | B. With pull-ups the knob rests (at a detent)
 * with both pins high: code 3. One detent is a full Gray-code cycle of 4 moves:
 *   CW:  3 -> 1 -> 0 -> 2 -> 3
 *   CCW: 3 -> 2 -> 0 -> 1 -> 3
 *
 * Each change between neighbouring codes adds +1 (CW) or -1 (CCW) to a running
 * total. Bounces go back and forth, so they cancel out.
 *
 * If a state passes between two snapshots, both pins appear to change at once
 * (a "jump" of two moves). The direction of a jump can't be read from the
 * pins, so it is counted as two moves in the direction the knob was last
 * turning.
 *
 * A step is reported as soon as the total reaches a full detent (+/-4), so a
 * missed rest state doesn't lose the click. When the knob reaches rest, a
 * partial total of at least +/-2 also counts as a step (a detent whose states
 * were partly missed), then the total is cleared to re-align with the detent.
 */
#define NUM_CODES 4
#define CODE_REST 3
#define MOVES_PER_DETENT 4
#define MIN_MOVES_AT_REST 2

#define CW  (+1)
#define CCW (-1)
#define NO_MOVE 0
#define JUMP 2  // both pins changed: two moves in an unknown direction

// TRANSITIONS[previous code][new code]
static const int TRANSITIONS[NUM_CODES][NUM_CODES] = {
    //  to 0      to 1      to 2      to 3
    { NO_MOVE,  CCW,      CW,       JUMP    },  // from 0
    { CW,       NO_MOVE,  JUMP,     CCW     },  // from 1
    { CCW,      JUMP,     NO_MOVE,  CW      },  // from 2
    { JUMP,     CW,       CCW,      NO_MOVE },  // from 3
};

static const unsigned int LINES[NUM_ROTARY_LINES] = {
    [LINE_A_INDEX] = ROTARY_LINE_A,
    [LINE_B_INDEX] = ROTARY_LINE_B,
};

static struct gpiod_line_request *request = NULL;

static int previousCode = CODE_REST;
static int movement = 0;         // sum of moves not yet reported as a step
static int lastDirection = CW;   // direction of the most recent single move

// Net steps decoded but not yet returned by Rotary_waitStep() (+ = CW, - = CCW)
static int pendingSteps = 0;

/* Decoder */
static int levelsToCode(int a, int b)
{
    return ((a ? 1 : 0) << 1) | (b ? 1 : 0);
}

/**
 * @brief Converts a pin-code change into a signed number of moves
 */
static int movesFor(int fromCode, int toCode)
{
    int transition = TRANSITIONS[fromCode][toCode];
    if (transition == JUMP) {
        return JUMP * lastDirection;
    }
    if (transition != NO_MOVE) {
        lastDirection = transition;
    }
    return transition;
}

static int directionToStep(int moves)
{
    return (moves > 0) ? ROTARY_STEP_CW : ROTARY_STEP_CCW;
}

/**
 * @brief Feeds the current pin levels into the decoder
 * @return ROTARY_STEP_CW or ROTARY_STEP_CCW when a detent completes, otherwise 0
 */
static int processLevels(int a, int b)
{
    int code = levelsToCode(a, b);
    movement += movesFor(previousCode, code);
    previousCode = code;

    // A full detent's worth of movement is a step, even if rest was missed
    if (movement >= MOVES_PER_DETENT || movement <= -MOVES_PER_DETENT) {
        int step = directionToStep(movement);
        movement -= step * MOVES_PER_DETENT;
        if (code == CODE_REST) {
            movement = 0;
        }
        return step;
    }

    if (code != CODE_REST) {
        return 0;
    }

    // Back at rest: accept a partial detent, then re-align with the detent
    int step = 0;
    if (movement >= MIN_MOVES_AT_REST || movement <= -MIN_MOVES_AT_REST) {
        step = directionToStep(movement);
    }
    movement = 0;
    return step;
}

/* Pins */
/**
 * @brief Reads both encoder pins at the same instant
 * @return true on success
 */
static bool readPins(int *a, int *b)
{
    int values[NUM_ROTARY_LINES];
    if (GPIO_ReadLines(request, LINES, NUM_ROTARY_LINES, values) != 0) {
        return false;
    }
    *a = values[LINE_A_INDEX];
    *b = values[LINE_B_INDEX];
    return true;
}

/**
 * @brief Hands out one queued step, if there is one
 * @return true if *step was set
 */
static bool takePendingStep(int *step)
{
    if (pendingSteps > 0) {
        pendingSteps--;
        *step = ROTARY_STEP_CW;
        return true;
    }
    if (pendingSteps < 0) {
        pendingSteps++;
        *step = ROTARY_STEP_CCW;
        return true;
    }
    return false;
}

/* Public functions */
int Rotary_init(void)
{
    if (request != NULL) {
        fprintf(stderr, "Rotary_init: already initialized\n");
        return -1;
    }

    unsigned int offsets[NUM_ROTARY_LINES] = {ROTARY_LINE_A, ROTARY_LINE_B};
    request = GPIO_Init(ROTARY_CHIP, offsets, NUM_ROTARY_LINES,
                        GPIOD_LINE_DIRECTION_INPUT, GPIOD_LINE_EDGE_BOTH,
                        ROTARY_DEBOUNCE_US);
    if (request == NULL) {
        return -1;
    }

    int a;
    int b;
    if (!readPins(&a, &b)) {
        Rotary_cleanup();
        return -1;
    }

    // Start from wherever the knob actually is
    previousCode = levelsToCode(a, b);
    movement = 0;
    lastDirection = CW;
    pendingSteps = 0;
    return 0;
}

void Rotary_cleanup(void)
{
    if (request != NULL) {
        gpiod_line_request_release(request);
        request = NULL;
    }
}

int Rotary_waitStep(int timeoutMs, int *step)
{
    if (request == NULL || step == NULL) {
        fprintf(stderr, "Rotary_waitStep: not initialized or bad argument\n");
        return -1;
    }

    if (takePendingStep(step)) {
        return 1;
    }

    // Wake on any edge, then decode from a snapshot of the real pin levels
    while (true) {
        GPIO_EdgeEvent events[EVENT_BATCH_SIZE];
        int numEvents = GPIO_WaitEdges(request, timeoutMs, events, EVENT_BATCH_SIZE);
        if (numEvents <= 0) {
            return numEvents;  // 0 = timeout, -1 = error
        }

        int a;
        int b;
        if (!readPins(&a, &b)) {
            return -1;
        }
        pendingSteps += processLevels(a, b);

        if (takePendingStep(step)) {
            return 1;
        }
    }
}
