#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"


// Light an LED strip from green -> purple -> orange in a loop, with smooth transitions, for Halloween area lighting.
// TODO: Eventually, we'll probably need to figure out how to control multiple strips.


// LED strip specs + wiring configuration
/**
 * Each randomFill is NUM_LEDS calls to show(); 60 calls * 2ms = 120ms
 * Each color transition is 128 steps, each step calling randomFill(); 128 * 120ms = ~15s
 *
 * Color -> color transition overhead is (FastLED.show() time) * (LED strip length / # simul. swaps) * (# fade steps)
 * 2.5ms * 60 * 128
 *
 * We could optimize this by:
 *  1. Reducing the number of times that randomFill calls show() (by switching more than one LED at a time)
 *  2. Reducing the amount of time show() takes (by using hardware SPI)
 *  3. Reducing the number of steps in a color transition (by adjusting theta by more than one at a time)
 *
 * Those optimizations are in order of preference. Hardware SPI sounds nice, until you think about how we'll need to
 * control multiple strips at some point.
 */
#define LED_TYPE APA102
#define LED_COLOR_ORDER BGR
#define NUM_LEDS 60
#define DATA_PIN 4
#define CLOCK_PIN 5
#define POWER_SYSTEM_VOLTAGE 5
#define POWER_SYSTEM_MAX_CURRENT_MA 3000


// LED color array
CRGB leds[NUM_LEDS];

// The colors we'll be using
#define HALLOWEEN_ORANGE_HUE 22
#define HALLOWEEN_PURPLE_HUE 189
#define HALLOWEEN_GREEN_HUE 96
#define HALLOWEEN_BRIGHTNESS 255
#define HALLOWEEN_SATURATION 255
#define COLOR_HOLD_TIME_MS 300000 // five minutes
CRGB halloween_orange = CHSV(HALLOWEEN_ORANGE_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);
CRGB halloween_purple = CHSV(HALLOWEEN_PURPLE_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);
CRGB halloween_green = CHSV(HALLOWEEN_GREEN_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);
CRGB color_sequence[] = { halloween_orange, halloween_green, halloween_purple };
size_t color_sequence_len = sizeof(color_sequence) / sizeof(color_sequence[0]);

// Array for occasional shuffling, for use with random-swap color transitions
uint8_t random_led_indices[NUM_LEDS];


/**
 * Initialize everything.
 */
void setup() {
    FastLED.setMaxPowerInVoltsAndMilliamps(POWER_SYSTEM_VOLTAGE, POWER_SYSTEM_MAX_CURRENT_MA);
    FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // Initialize our random-index array
    for (uint8_t i=0; i<NUM_LEDS; i++) {
        random_led_indices[i] = i;
    }
}


/**
 * Shuffle all of the elements in an array. O(n).
 */
void shuffleArray(uint8_t* arr, size_t array_len) {
    for (size_t swap_index_1=array_len-1; swap_index_1 >= 1; swap_index_1--) {
        uint8_t swap_index_2 = random(array_len);
        // Swap swap_index_1 and swap_index_2
        uint8_t tmp = arr[swap_index_1];
        arr[swap_index_1] = arr[swap_index_2];
        arr[swap_index_2] = tmp;
    }
}


/**
 * Fill all colors in the given array to a new color, one random index at a time, show()ing after each swap.
 *
 * Designed to provide a smoother transition than an all-at-once fill, like that provided by fill_solid().
 */
void randomFill(CRGB* leds, size_t num_leds, CRGB new_color, uint8_t num_simultaneous_swaps,
                uint16_t intra_swap_delay_us)
{
    shuffleArray(random_led_indices, num_leds);

    size_t num_swaps_made = 0;
    while (num_swaps_made < num_leds) {
        for (int i=0; i<num_simultaneous_swaps && num_swaps_made<num_leds; i++) {
            leds[random_led_indices[num_swaps_made]] = new_color;
            num_swaps_made += 1;
        }
        FastLED.show();

        if (intra_swap_delay_us > 0) {
            delayMicroseconds(intra_swap_delay_us);
        }
    }
}

#define THETA_START 191 // start at 270 deg. around the unit circle
#define THETA_END 63 // end at 90 deg. around the unit circle
CRGB color_of_origin = color_sequence[0];
CRGB destination_color = color_sequence[1];
size_t color_sequence_index = 1;
uint8_t theta = THETA_START;

void loop() {
    uint8_t percent_toward_destination = sin8(theta);
    CRGB fill_color = blend(color_of_origin, destination_color, percent_toward_destination);
    randomFill(leds, NUM_LEDS, fill_color, 1, 30);
    // randomFill() calls show() for us

    if (theta == THETA_END) {
        // We have arrived at our destination color.
        // Mark what used to be our destination as our origin, and identify our next destination.
        // TODO: Eventually, we'll want to hold onto each destination for a minute or more.
        color_of_origin = destination_color;
        color_sequence_index += 1;
        if (color_sequence_index >= color_sequence_len) {
            color_sequence_index = 0;
        }
        destination_color = color_sequence[color_sequence_index];

        theta = THETA_START;

        delay(COLOR_HOLD_TIME_MS);
    } else {
        // We have not yet arrived at our destination color.
        // Progress toward it.
        theta += 1; // WARNING: This relies on overflow
    }

    delay(1);
}
