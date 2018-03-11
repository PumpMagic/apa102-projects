#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

// Program specs:
// Light an LED strip from green -> purple -> orange in a loop, with smooth transitions, for Halloween area lighting


// LED strip specs + wiring configuration
// TODO: Consider switching to hardware SPI to speed things up. Color transition times are dominated by calls to FastLED.show().
#define LED_TYPE APA102
#define NUM_LEDS 60
#define LED_COLOR_ORDER BGR
#define DATA_PIN 4
#define CLOCK_PIN 5


// LED color array
CRGB leds[NUM_LEDS];

// Array for occasional shuffling, for use with random-swap color transitions
uint8_t random_led_indices[NUM_LEDS];

// Halloween colors
#define HALLOWEEN_ORANGE_HUE 22
#define HALLOWEEN_PURPLE_HUE 189
#define HALLOWEEN_GREEN_HUE 96
#define HALLOWEEN_BRIGHTNESS 255
#define HALLOWEEN_SATURATION 255
CRGB halloween_orange = CHSV(HALLOWEEN_ORANGE_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);
CRGB halloween_purple = CHSV(HALLOWEEN_PURPLE_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);
CRGB halloween_green = CHSV(HALLOWEEN_GREEN_HUE, HALLOWEEN_SATURATION, HALLOWEEN_BRIGHTNESS);


/**
 * Initialize everything
 * 
 * TODO: Call setMaxPowerInVoltsAndMilliamps()?
 */
void setup() {
   FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

   // Initialize our random-index array
   for (int i=0; i<NUM_LEDS; i++) {
    random_led_indices[i] = i;
   }
}


/**
 * Shuffle all of the elements in an array. O(n).
 * 
 * Implements Fisher-Yates shuffle: https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
 */
void shuffleArray(uint8_t* arr, size_t array_len) {
  uint8_t swap_index_2 = 0;
  for (int swap_index_1=array_len-1; swap_index_1 >= 1; swap_index_1--) {
    swap_index_2 = random(array_len);
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
    
    if (i < num_leds-1) {
      //delayMicroseconds(intra_swap_delay_us);
    }
  }
}


uint8_t curve_location = 0;
CRGB sequence[] = { halloween_orange, halloween_green, halloween_purple };
size_t sequence_index = 0;

// TODO: Initialize these from the sequence array
CRGB left = halloween_orange;
CRGB right = halloween_green;

void loop() {
  uint8_t bias = sin8(curve_location);
  CRGB color = blend(left, right, bias);
    
//  fill_solid(leds, NUM_LEDS, color);
  randomFill(leds, NUM_LEDS, color, 100);

  FastLED.show();

  // TODO: Clean up this logic. We shouldn't be relying on the phase of FastLED's sin8.
  // Can we use some other way to detect that we've fully transitioned from one color to another?
  if (curve_location == 63) {
    sequence_index += 1;
    if (sequence_index > 2) { // TODO: Use length of array
      sequence_index = 0;
    }
    left = sequence[sequence_index];
  } else if (curve_location == 191) {
    sequence_index += 1;
    if (sequence_index > 2) { // TODO: Use length of array
      sequence_index = 0;
    }
    right = sequence[sequence_index];
  }
  
  curve_location += 1;
  
  //delay(1);
}
