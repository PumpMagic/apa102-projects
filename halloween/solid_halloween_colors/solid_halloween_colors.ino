#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"


// Light an LED strip from green -> purple -> orange in a loop, with smooth transitions, for Halloween area lighting.
// TODO: Eventually, we'll probably need to figure out how to control multiple strips.


// LED strip specs + wiring configuration
// TODO: Consider switching to hardware SPI to speed things up. Color transition times are currently dominated by calls
// to FastLED.show(), which takes around 2-3 milliseconds
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
CRGB halloween_orange = CHSV(HALLOWEEN_ORANGE_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);
CRGB halloween_purple = CHSV(HALLOWEEN_PURPLE_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);
CRGB halloween_green = CHSV(HALLOWEEN_GREEN_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);
CRGB color_sequence[] = { halloween_orange, halloween_green, halloween_purple };

// Array for occasional shuffling, for use with random-swap color transitions
uint8_t random_led_indices[NUM_LEDS];


/**
 * Initialize everything
 */
void setup() {
    FastLED.setMaxPowerInVoltsAndMilliamps(POWER_SYSTEM_VOLTAGE, POWER_SYSTEM_MAX_CURRENT_MA);
    FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // Initialize our random-index array
    for (uint8_t i=0; i<NUM_LEDS; i++) {
        random_led_indices[i] = i;
    }

//    Serial.begin(9200);
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
void randomFill(CRGB* leds, size_t num_leds, CRGB new_color, uint16_t intra_swap_delay_us) {
    shuffleArray(random_led_indices, num_leds);

    for (int i=0; i<num_leds; i++) {
        leds[random_led_indices[i]] = new_color;

        FastLED.show();

        if (i < num_leds-1 && intra_swap_delay_us > 0) {
            delayMicroseconds(intra_swap_delay_us);
        }
    }
}


CRGB color_of_origin = color_sequence[0];
CRGB destination_color = color_sequence[1];
size_t color_sequence_index = 1;
uint8_t theta = 191;

void loop() {
    uint8_t bias_toward_destination = sin8(theta);
    CRGB fill_color = blend(color_of_origin, destination_color, bias_toward_destination);

//    fill_solid(leds, NUM_LEDS, fill_color);
    randomFill(leds, NUM_LEDS, fill_color, 100);

    // randomFill() calls show() for us
    //FastLED.show();

    // TODO: Clean up this logic. Do we need to rely on the phase of FastLED's sin8?
    // Can we use some other way to detect that we've fully transitioned from one color to another?
    if (theta == 63) {
        // We have arrived at our destination color.
        // Mark what used to be our destination as our origin, and identify our next destination.
        // TODO: Eventually, we'll want to hold onto each destination for a minute or more.
        // Can probably get away with a call to delay(), for now. Deal with millis overflow? ehhh...
        color_of_origin = destination_color;
        color_sequence_index += 1;
        if (color_sequence_index > 2) { // TODO: Use length of array
            color_sequence_index = 0;
        }
        destination_color = color_sequence[color_sequence_index];

        // TODO: Magic number
        theta = 191;
    } else {
        // TODO: This relies on overflow
        theta += 1;
    }

    //delay(1);
}
