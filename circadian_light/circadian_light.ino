#include <time.h>
#include <Wire.h>
#include "RTClib.h"
#include "FastLED.h"

/**
 * circadian_light: Software that controls LEDs to mimic the sun.
 * 
 * Requires an RTClib-compatible RTC and a FastLED-compatible sequence of LEDs.
 */

// Useful links:
//  time.h docs: https://www.nongnu.org/avr-libc/user-manual/group__avr__time.html
//  TimeLord sunrise / sunset time example: https://github.com/probonopd/TimeLord/blob/master/examples/SunriseSunset/SunriseSunset.ino
//  time.h avr history: http://swfltek.com/arduino/timelord.html

// Location of the observer
// (GPS coordinates of the light we're controlling)
#define LATITUDE 37.0
#define LONGITUDE -122.0

// RTC time offset
// TODO: Actually use this. Right now, we assume the user has programmed the RTC as UTC.
#define UTC_OFFSET_HOURS -7

// What to reset the RTC with if it's lost power
#define RTC_RESET_YEAR    2017
#define RTC_RESET_MONTH   1
#define RTC_RESET_DAY     1
#define RTC_RESET_HOUR    0
#define RTC_RESET_MINUTE  0
#define RTC_RESET_SECOND  0

// LED stuff
#define LED_TYPE APA102
#define LED_COLOR_ORDER BGR
#define NUM_LEDS 2
#define DATA_PIN 4
#define CLOCK_PIN 5
#define POWER_SYSTEM_VOLTAGE 5
#define POWER_SYSTEM_MAX_CURRENT_MA 3000


// Shared data structures
static RTC_DS3231 rtc;
static CRGB leds[NUM_LEDS];


void setup() {
  // Set up the serial port
  Serial.begin(9600);
  Serial.println("Hi!\n");

  // Set up the RTC
  rtc.begin();
  
  // If the RTC's lost power, reset it
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(RTC_RESET_YEAR, RTC_RESET_MONTH, RTC_RESET_DAY,
                        RTC_RESET_HOUR, RTC_RESET_MINUTE, RTC_RESET_SECOND));
  }

  // Set up our LEDs
  FastLED.setMaxPowerInVoltsAndMilliamps(POWER_SYSTEM_VOLTAGE, POWER_SYSTEM_MAX_CURRENT_MA);
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Set up our time library
  set_position(LATITUDE * ONE_DEGREE, LONGITUDE * ONE_DEGREE);
  // set_zone doesn't seem to affect the output of sun_rise() or sun_set(), so don't bother
  // set_zone(UTC_OFFSET_HOURS * ONE_HOUR);
}

/**
 * Get noon of the given date, represented as a time_t.
 * 
 * This conversion (and rounding) are useful for working with avr-libc's time.h.
 * The rounding is useful because for whatever reason, time.h's returned sunrise/sunset times
 * vary by about 90 seconds for a single date, depending on what time you pass in.
 */
time_t toRoundedTimeT(DateTime date_time) {
  struct tm time_tm_noon;

  time_tm_noon.tm_isdst = 0; // our calculations expect there to be no DST; we're working with the sun
  time_tm_noon.tm_yday = -1; // time library's sun functions ignore this
  time_tm_noon.tm_wday = -1; // time library's sun functions ignore this
  time_tm_noon.tm_year = date_time.year() - 1900; // datetime returns 0-based, tm_year is 1900-based
  time_tm_noon.tm_mon = date_time.month() - 1; // datetime is 1-based, tm_mon is 0-based
  time_tm_noon.tm_mday = date_time.day(); // datetime and tm_mday are both 1-based
  time_tm_noon.tm_hour = 12;
  time_tm_noon.tm_min = 0;
  time_tm_noon.tm_sec = 0;

  time_t time_t_noon = mktime(&time_tm_noon);

  return time_t_noon;
}

/**
 * Convert a DateTime to a time_t.
 */
time_t toTimeT(DateTime date_time) {
  return (time_t)(date_time.secondstime());
}

/**
 * Get the sunrise time given a date.
 * 
 * The given date_time's date is important; its time is not.
 */
time_t getSunriseTime(DateTime date_time) {
  time_t rounded_time_t = toRoundedTimeT(date_time);
  time_t sunrise_time = sun_rise(&rounded_time_t);

  return sunrise_time;
}

/**
 * Get the sunset time given a date.
 * 
 * The given date_time's date is important; its time is not.
 */
time_t getSunsetTime(DateTime date_time) {
  time_t rounded_time_t = toRoundedTimeT(date_time);
  time_t sunset_time = sun_set(&rounded_time_t);

  return sunset_time;
}

/**
 * Approximate the sun's color given the current date and time
 */
CRGB getSunColor(DateTime current_date_time) {
  // Safe assumptions:
  //  The sun is either set or not set.
  //  The sun will rise or set either zero or one times per day.
  // Counterintuitive facts:
  //  Sunset can happen earlier on a calendar day than sunrise.
  //  The sun can go multiple days without rising or setting.
  //  The sun does not necessarily fully set after civil twilight.

  // As a first pass, we should code against none of those counterintuitive facts.
  // Specifically, code with these assumptions in mind:
  //  The sun is always set at midnight.
  //  The sun rises once a day.
  //  The sun sets once a day, later in the day than it rose.
  // This makes our logic a lot simpler, and lets us avoid coding around edge cases of avr-libc's time.h.
  // The downside is that our light won't work at specific geographic locations, such as the north and south poles.
  
  // TODO: Implement this
  //  1. Nighttime: we're in this state when the time is in the range [time.h's sunset plus delta, time.h's sunrise minus delta).
  //  2. Sunrise: when the time is in the range [time.h's sunrise minus delta, time.h's sunrise plus delta).
  //      lights are in the range (off, max daylight color) depending on the time.
  //  3. Daytime:when the time is in the range [time.h's sunrise plus delta, time.h's sunset minus delta).
  //  4. Sunset: lights transitioning from max daylight color to off. State in [time.h's sunset minus delta, time.h's sunset plus delta).
  // State transitions are 1 -> 2 -> 3 -> 4. Maybe near the poles we'd need a different state machine. Assume we aren't, for simplicity.
  // "Color", above, refers to the LEDs' hue, saturation, and value.
  // Go from about 2000K to 5500K. Try starting with (255, 147, 41) to (255, 255, 251).
  return CRGB::Black;
}

void loop() {
  // Get the time from the RTC
  DateTime current_time = rtc.now();

  // Get the sun-approximating color given the time of day
  CRGB sun_color = getSunColor(current_time);

  // Prep our LED color array
  fill_solid(leds, NUM_LEDS, sun_color);

  // Write to our LEDs
  FastLED.show();
  
  // Chill for a sec
  delay(1000);
}


//char serial_buffer[128] = "";
//sprintf(serial_buffer, "Sunrise over there is %lu (UTC)", sunrise_time);
//Serial.println(serial_buffer);
//sprintf(serial_buffer, "(%lu, local time)", sunrise_time + UTC_OFFSET_HOURS * ONE_HOUR);
//Serial.println(serial_buffer);
//sprintf(serial_buffer, "Sunset over there is %lu", sunset_time);
//Serial.println(serial_buffer);

