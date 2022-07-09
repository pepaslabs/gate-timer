// Beam interrupt ("gate timer") with 3-digit 7-segment display.
// https://github.com/pepaslabs/gate-timer
// Copyright 2022 Jason Pepas.
// Released under the terms of the MIT License, see https://opensource.org/licenses/MIT

// See https://create.arduino.cc/projecthub/akarsh98/controlling-7-segment-display-using-arduino-and-74hc595-52f09c
// See https://www.ti.com/product/SN74HC595
// See https://www.ti.com/product/ULN2003A

// A note on (hungarian) notation:
// g_foo indicates a global variable.
// foo_t indicates a type name.

// MARK: - Includes.

#include <stdint.h>
#include "types.h"


// MARK: - Pin assignments.

const pin_t g_clock_pin = 9;  // The 74HC595 clock pin (SRCLK, pin 11).
const pin_t g_latch_pin = 8;  // The 74HC595 latch pin (RCLK, pin 12).
const pin_t g_data_pin = 6;  // The 74HC595 data pin (SER, pin 14).
const pin_t g_trigger_pin = 7;  // The beam interrupt pin.
const pin_t g_decimal1_pin = 5;  // Most significant decimal point (left).
const pin_t g_decimal2_pin = 4;  // Middle decimal point.
const pin_t g_decimal3_pin = 3;  // Least significant decimal point (right).


// MARK: - 7-segment display.

// bits map to 74HC595 pins: Q7 Q6 Q5 Q4 Q3 Q2 Q1 Q0
#define SEGMENT_A_TOP         (0b01000000)
#define SEGMENT_B_UPPER_RIGHT (0b00100000)
#define SEGMENT_C_LOWER_RIGHT (0b00000100)
#define SEGMENT_D_BOTTOM      (0b00001000)
#define SEGMENT_E_LOWER_LEFT  (0b00010000)
#define SEGMENT_F_UPPER_LEFT  (0b00000010)
#define SEGMENT_G_CENTER      (0b00000001)

#define SEG_TO (SEGMENT_A_TOP)
#define SEG_UR (SEGMENT_B_UPPER_RIGHT)
#define SEG_LR (SEGMENT_C_LOWER_RIGHT)
#define SEG_BO (SEGMENT_D_BOTTOM)
#define SEG_LL (SEGMENT_E_LOWER_LEFT)
#define SEG_UL (SEGMENT_F_UPPER_LEFT)
#define SEG_CE (SEGMENT_G_CENTER)

// 7-segment number definitions, 0-9 (without dot).
#define NUMBER_0 (0b01111110)
#define NUMBER_1 (0b00100100)
#define NUMBER_2 (0b01111001)
#define NUMBER_3 (0b01101101)
#define NUMBER_4 (0b00100111)
#define NUMBER_5 (0b01001111)
#define NUMBER_6 (0b01011111)
#define NUMBER_7 (0b01100100)
#define NUMBER_8 (0b01111111)
#define NUMBER_9 (0b01101111)

const digit_t g_numbers[10] = {
    NUMBER_0, NUMBER_1, NUMBER_2, NUMBER_3, NUMBER_4,
    NUMBER_5, NUMBER_6, NUMBER_7, NUMBER_8, NUMBER_9
};

// Add a dot to a 7-segment digit.
digit_t dotted(digit_t digit) {
    return digit | 0b10000000;
}

// Remove the dot to a 7-segment digit.
digit_t undotted(digit_t digit) {
    return digit & 0b01111111;
}

// Return true if the digit is dotted.
bool has_dot(digit_t digit) {
    return digit & 0b10000000;
}

// Turn on a decimal point.  Note: 0 == least significant (right).
void turn_on_dot(uint8_t index) {
    if (index == 0) {
        digitalWrite(g_decimal3_pin, LOW); // inverted logic.
    } else if (index == 1) {
        digitalWrite(g_decimal2_pin, LOW); // inverted logic.
    } else if (index == 2) {
        digitalWrite(g_decimal1_pin, LOW); // inverted logic.
    }
}

// Turn off a decimal point.  Note: 0 == least significant (right).
void turn_off_dot(uint8_t index) {
    if (index == 0) {
        digitalWrite(g_decimal3_pin, HIGH); // inverted logic.
    } else if (index == 1) {
        digitalWrite(g_decimal2_pin, HIGH); // inverted logic.
    } else if (index == 2) {
        digitalWrite(g_decimal1_pin, HIGH); // inverted logic.
    }
}

// Utility function to display (the 3 LSD's of) an arbitrary integer.
void display_int(int number) {
    digit_t display_state[3];
    uint8_t hundreds = 0;
    uint8_t tens = 0;
    uint8_t ones = 0;

    int tmp = number;
    ones = tmp % 10;
    if (number >= 10) {
        tmp = tmp / 10;
        tens = tmp % 10;
        if (tmp >= 100) {
            tmp = tmp / 10;
            hundreds = tmp % 10;
        }
    }

    display_state[2] = g_numbers[hundreds];
    display_state[1] = g_numbers[tens];
    display_state[0] = g_numbers[ones];
    send_three_digits(display_state);
}

// The currently displayed digits.
// Note: g_display_state[0] == least significant (right).
digit_t g_display_state[3] = { 9, 9, 9 };

// Serially shift a digit out to the chain of 7-segment displays.
void shift_out_digit(digit_t digit) {
    digitalWrite(g_latch_pin, LOW);
    shiftOut(g_data_pin, g_clock_pin, MSBFIRST, digit & 0b01111111);
    digitalWrite(g_latch_pin, HIGH);
    g_display_state[0] = g_display_state[1];
    g_display_state[1] = g_display_state[2];
    g_display_state[2] = digit;
    for (uint8_t i = 0; i < 3; i++) {
        if (has_dot(g_display_state[i])) {
            turn_on_dot(i);
        } else {
            turn_off_dot(i);
        }
    }
}

// Update all three digits of the display.
void send_three_digits(digit_t* digits) {
    digitalWrite(g_latch_pin, LOW);
    shiftOut(g_data_pin, g_clock_pin, MSBFIRST, digits[0] & 0b01111111);
    shiftOut(g_data_pin, g_clock_pin, MSBFIRST, digits[1] & 0b01111111);
    shiftOut(g_data_pin, g_clock_pin, MSBFIRST, digits[2] & 0b01111111);
    digitalWrite(g_latch_pin, HIGH);
    g_display_state[0] = digits[0];
    g_display_state[1] = digits[1];
    g_display_state[2] = digits[2];
   for (uint8_t i = 0; i < 3; i++) {
       if (has_dot(g_display_state[i])) {
           turn_on_dot(i);
       } else {
           turn_off_dot(i);
       }
   }
}


// MARK: - Beam interrupt and timer logic.

// Is there an unhandled gate interrupt?
volatile bool g_pending_gate_interrupt = false;

const millis_t g_run_in_threshold = 2000;
const millis_t g_run_out_threshold = 2000;

millis_t g_millis_of_last_interrupt = 0;
millis_t g_millis_of_last_timer_start = 0;
millis_t g_millis_of_last_timer_stop = 0;
millis_t g_millis_of_current_loop = 0;

// The beam interrupt service routine.
void gate_ISR() {
    g_pending_gate_interrupt = true;
}

// Timer states:
// The gate timer is stopped.
#define TIMER_STATE_STOPPED 0
// The gate timer has been started and is not yet stoppable.
#define TIMER_STATE_RUN_IN 1
// The gate timer is running.
#define TIMER_STATE_RUNNING 2
// The gate timer has been stopped and is not yet startable.
#define TIMER_STATE_RUN_OUT 3

uint8_t g_timer_state = TIMER_STATE_STOPPED;

// Start the timer.
void start_timer() {
    g_timer_state = TIMER_STATE_RUN_IN;
    g_millis_of_last_timer_start = g_millis_of_last_interrupt;
}

// Stop the timer.
void stop_timer() {
    g_timer_state = TIMER_STATE_RUN_OUT;
    g_millis_of_last_timer_stop = g_millis_of_last_interrupt;
}

// Process a beam interruption.
void gate_did_trigger() {
    switch(g_timer_state) {
        case TIMER_STATE_STOPPED:
            start_timer();
            break;
        case TIMER_STATE_RUN_IN:
            break;
        case TIMER_STATE_RUNNING:
            stop_timer();
            break;
        case TIMER_STATE_RUN_OUT:
            break;
    }
}

// Process changes to the timer state.
void update_timer_state() {
    switch(g_timer_state) {
        case TIMER_STATE_STOPPED:
            break;
        case TIMER_STATE_RUN_IN:
            {
                millis_t run_in = g_millis_of_current_loop - g_millis_of_last_timer_start;
                if (run_in >= g_run_in_threshold) {
                    g_timer_state = TIMER_STATE_RUNNING;
                }
            }
            break;
        case TIMER_STATE_RUNNING:
            break;
        case TIMER_STATE_RUN_OUT:
            {
                millis_t run_out = g_millis_of_current_loop - g_millis_of_last_timer_stop;
                if (run_out > g_run_out_threshold) {
                    g_timer_state = TIMER_STATE_STOPPED;
                }
            }
            break;
    }
}

// Return elapsed milliseconds of an active or stopped timer.
millis_t get_elapsed_timer_millis() {
    if (g_timer_state == TIMER_STATE_STOPPED || g_timer_state == TIMER_STATE_RUN_OUT) {
        return g_millis_of_last_timer_stop - g_millis_of_last_timer_start;
    } else {
        return g_millis_of_current_loop - g_millis_of_last_timer_start;
    }
}

// Convert milliseconds into an array of three digit_t's.
void millis_as_display_state(millis_t elapsed, digit_t* digits_out) {
    uint8_t hundreds = 0;
    uint8_t tens = 0;
    uint8_t ones = 0;
    uint8_t tenths = 0;

    millis_t tmp = elapsed;
    if (elapsed >= 100) {
        tmp = tmp / 100;
        tenths = tmp % 10;

        if (elapsed >= 1000) {
            tmp = tmp / 10;
            ones = tmp % 10;

            if (elapsed >= 10000) {
                tmp = tmp / 10;
                tens = tmp % 10;

                if (elapsed >= 100000) {
                    tmp = tmp / 10;
                    hundreds = tmp % 10;
                }
            }
        }
    }

    if (hundreds > 0) {
        digits_out[2] = g_numbers[hundreds];
        digits_out[1] = g_numbers[tens];
        digits_out[0] = dotted(g_numbers[ones]);
    } else {
        digits_out[2] = g_numbers[tens];
        digits_out[1] = dotted(g_numbers[ones]);
        digits_out[0] = g_numbers[tenths];
    }
}

// Compare two arrays of digit_t's.
bool display_states_are_equal(digit_t* a, digit_t* b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

// Shift out the current display digits if they have changed.
void update_display_if_needed() {
    millis_t elapsed = get_elapsed_timer_millis();
    digit_t proposed_display_state[3];
    millis_as_display_state(elapsed, proposed_display_state);
    if (!display_states_are_equal(g_display_state, proposed_display_state)) {
        send_three_digits(proposed_display_state);
    }
}


// MARK: - Animations

void snake_animation(millis_t delay_ms) {
    digit_t frames[8] = {
        SEG_TO, SEG_UR, SEG_CE, SEG_LL,
        SEG_BO, SEG_LR, SEG_CE, SEG_UL
    };
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(g_latch_pin, LOW);
        shiftOut(g_data_pin, g_clock_pin, MSBFIRST, frames[i]);
        digitalWrite(g_latch_pin, HIGH);
        delay(delay_ms);
    }
}

void circle_animation(millis_t delay_ms) {
    digit_t frames[6] = { SEG_TO, SEG_UR, SEG_LR, SEG_BO, SEG_LL, SEG_UL };
    for (uint8_t i = 0; i < 6; i++) {
        digitalWrite(g_latch_pin, LOW);
        shiftOut(g_data_pin, g_clock_pin, MSBFIRST, frames[i]);
        digitalWrite(g_latch_pin, HIGH);
        delay(delay_ms);
    }
}

void numeric_animation(millis_t delay_ms) {
    digit_t frames[10] = {
        NUMBER_0, NUMBER_1, NUMBER_2, NUMBER_3, NUMBER_4,
        NUMBER_5, NUMBER_6, NUMBER_7, NUMBER_8, NUMBER_9,
    };
    for (uint8_t i = 0; i < 10; i++) {
        digitalWrite(g_latch_pin, LOW);
        shiftOut(g_data_pin, g_clock_pin, MSBFIRST, frames[i]);
        digitalWrite(g_latch_pin, HIGH);
        delay(delay_ms);
    }
}

void numeric_animation2(millis_t delay_ms) {
    digit_t frames[10] = {
        NUMBER_0, NUMBER_1, NUMBER_2, NUMBER_3, NUMBER_4,
        NUMBER_5, NUMBER_6, NUMBER_7, NUMBER_8, NUMBER_9,
    };
    for (uint8_t i = 0; i < 10; i++) {
        shift_out_digit(frames[i]);
        shift_out_digit(frames[i]);
        shift_out_digit(frames[i]);
        delay(delay_ms);
    }
}

void dot_animation(millis_t delay_ms) {
    turn_on_dot(0);
    delay(delay_ms);
    turn_off_dot(0);
    turn_on_dot(1);
    delay(delay_ms);
    turn_off_dot(1);
    turn_on_dot(2);
    delay(delay_ms);
    turn_off_dot(2);
    turn_on_dot(3);
    delay(delay_ms);
    turn_off_dot(3);
    delay(delay_ms);
}


// MARK: - Arduino runtime.

void setup() {
    pinMode(g_trigger_pin, INPUT);

    pinMode(g_latch_pin, OUTPUT);
    pinMode(g_clock_pin, OUTPUT);
    pinMode(g_data_pin, OUTPUT);

    pinMode(g_decimal1_pin, OUTPUT);
    turn_off_dot(0);
    pinMode(g_decimal2_pin, OUTPUT);
    turn_off_dot(1);
    pinMode(g_decimal3_pin, OUTPUT);
    turn_off_dot(2);

    // boot-up animation
    for(uint8_t i = 0; i < 2; i++) {
        snake_animation(50);
    }

    // Initialize display to "00.0"
   shift_out_digit(g_numbers[0]);
   shift_out_digit(dotted(g_numbers[0]));
   shift_out_digit(g_numbers[0]);

    attachInterrupt(digitalPinToInterrupt(g_trigger_pin), gate_ISR, FALLING);
}

// Call this from loop() to run an animation.
void loop__animation() {
    // Uncomment one of the following:
    // snake_animation(50); return;
    // circle_animation(50); return;
    // dot_animation(100); return;
    // numeric_animation(250); return;
    // numeric_animation2(250); return;
}

// Call this from loop() to display the count of interrupts.
void loop__interrupt_test() {
    static int interrupt_count = -1;
    if (interrupt_count == -1) {
        interrupt_count += 1;
        display_int(interrupt_count);
    }
    if (g_pending_gate_interrupt) {
        g_pending_gate_interrupt = false;
        interrupt_count += 1;
        display_int(interrupt_count);
    }
}

// Call this from loop() for "normal mode of operation".
void loop__gate_timer() {
    if (g_pending_gate_interrupt) {
        g_pending_gate_interrupt = false;
        g_millis_of_last_interrupt = millis();
        gate_did_trigger();
    } else {
        g_millis_of_current_loop = millis();
        update_timer_state();
        update_display_if_needed();
    }
}

void loop() {
    // Uncomment one of the following:
    // loop__animation(); return;
    // loop__interrupt_test(); return;
     loop__gate_timer(); return;
}
