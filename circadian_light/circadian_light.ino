#include <time.h>
#include <Wire.h>
#include "RTClib.h"
#include "FastLED.h"
#include <math.h>

/**
 * circadian_light: Software that controls LEDs to mimic the sun.
 * 
 * Requires an RTClib-compatible RTC and a FastLED-compatible sequence of LEDs.
 */

// Useful links:
//  time.h docs: https://www.nongnu.org/avr-libc/user-manual/group__avr__time.html
//  TimeLord sunrise / sunset time example: https://github.com/probonopd/TimeLord/blob/master/examples/SunriseSunset/SunriseSunset.ino
//  time.h avr history: http://swfltek.com/arduino/timelord.html

// Uncomment this, unless we're running in debug mode
#define DEBUG

// Location of the observer
// (GPS coordinates of the light we're controlling)
#define LATITUDE 37.0
#define LONGITUDE -122.0

// RTC time offset
// TODO: Actually use this. Right now, we assume the user has programmed the RTC as UTC.
#define UTC_OFFSET_HOURS -7
#define SECONDS_PER_HOUR 3600

// What to reset the RTC with if it's lost power
#define RTC_RESET_YEAR    2017
#define RTC_RESET_MONTH   1
#define RTC_RESET_DAY     1
#define RTC_RESET_HOUR    0
#define RTC_RESET_MINUTE  0
#define RTC_RESET_SECOND  0

// LED stuff
#define LED_TYPE APA102
#define LED_COLOR_ORDER RBG
#define NUM_LEDS 1
#define DATA_PIN 3
#define CLOCK_PIN 13
#define POWER_SYSTEM_VOLTAGE 5
#define POWER_SYSTEM_MAX_CURRENT_MA 3000


// How long we take to go from zero light to full light
// In the real world this transition isn't a constant for every day, it varies.
#define SUNRISE_TRANSITION_TIME_SECONDS 3600 // an hour
#define SUNSET_TRANSITION_TIME_SECONDS SUNRISE_TRANSITION_TIME_SECONDS

// TODO: FastLED's blend() probably isn't sufficient for our use.
// We want to go from orange to white along the color spectrum, not along the RGB space.
// Check this out: http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
#define HORIZON_HUE 21
#define HORIZON_SATURATION 214
#define DAY_HUE 42
#define DAY_SATURATION 4
#define NIGHT_HUE 0
#define NIGHT_SATURATION 0
CRGB HORIZON_COLOR = CHSV(HORIZON_HUE, HORIZON_SATURATION, 255);
CRGB DAY_COLOR = CHSV(DAY_HUE, DAY_SATURATION, 255);
CRGB NIGHT_COLOR = CHSV(NIGHT_HUE, NIGHT_SATURATION, 0);


// Shared data structures
static RTC_DS3231 rtc;
static CRGB leds[NUM_LEDS];
static char serial_buffer[128] = "";


void setup() {
  // Set up the serial port
  Serial.begin(9600);
  Serial.println("Hi!\n");

  #ifndef DEBUG
  // Set up the RTC
  rtc.begin();
  
  // If the RTC's lost power, reset it
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(RTC_RESET_YEAR, RTC_RESET_MONTH, RTC_RESET_DAY,
                        RTC_RESET_HOUR, RTC_RESET_MINUTE, RTC_RESET_SECOND));
  }
  #endif

  // Set up our LEDs
  FastLED.setMaxPowerInVoltsAndMilliamps(POWER_SYSTEM_VOLTAGE, POWER_SYSTEM_MAX_CURRENT_MA);
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  #ifdef DEBUG
  CRGB test_color = CHSV(HORIZON_HUE, HORIZON_SATURATION, 128);
  fill_solid(leds, NUM_LEDS, test_color);
  FastLED.show();
//  while (true) { delay(1); }
  #endif

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
 * Convert a DateTime to a time_t.
 */
time_t toTimeT(DateTime date_time) {
  return (time_t)(date_time.secondstime());
}


// Only works from 1000K to 6600K.
// C implementation of http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
CRGB colorTemperatureToRGB(uint16_t temperature_kelvin) {
    uint16_t temperature_scaled = temperature_kelvin / 100;

    uint8_t red = 255;

    uint8_t green = 0;
    double green_start = log(temperature_scaled);
    double green_raw = (99.4708025861 * green_start) - 161.1195681661;
    if (green_raw < 0) {
        green = 0;
    } else if (green_raw > 255) { 
        green = 255;
    } else {
        green = green_raw;
    }

    uint8_t blue = 0;
    if (temperature_scaled <= 19) {
        blue = 0;
    } else {
        double blue_start = log(temperature_scaled - 10);
        double blue_raw = (138.5177312231 * blue_start) - 305.0447927307;
        if (blue_raw < 0) {
            blue = 0;
        } else if (blue_raw > 255) {
            blue = 255;
        } else {
            blue = blue_raw;
        }
    }


    sprintf(serial_buffer, "RGB %u %u %u", red, green, blue);
    Serial.println(serial_buffer);
    return CRGB(red, green, blue);
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

  // TODO as of April 3: Twilight is only really orange at the horizon. Looking straight up at the sky, one just sees
  // a white / blue brightness change. This is consistent with the explanation that the atmosphere bends certain colors.
  // This undermines the importance of the color temperature gradient, and suggests the more relevant factor is brightness.
  // Still: start at 2000K really dim, maybe hang out there for a few minutes, and then just adjust brightness + temperature up simultaneously.

  TimeSpan timezone_offset = TimeSpan(UTC_OFFSET_HOURS * SECONDS_PER_HOUR);
  // TODO: Better understand / document timezone issues. We add our timezone offset here to avoid jumping prematurely to the
  // next day, since avr-libc's sunrise seems to only like UTC.
  DateTime sun_time_query = current_date_time + timezone_offset;
  
  time_t sunrise_time = getSunriseTime(sun_time_query);
  time_t sunset_time = getSunsetTime(sun_time_query);
  time_t current_time = toTimeT(current_date_time);

  time_t sunrise_transition_start = sunrise_time - (SUNRISE_TRANSITION_TIME_SECONDS / 2);
  time_t sunrise_transition_end = sunrise_time + (SUNRISE_TRANSITION_TIME_SECONDS / 2);
  time_t sunset_transition_start = sunset_time - (SUNSET_TRANSITION_TIME_SECONDS / 2);
  time_t sunset_transition_end = sunset_time + (SUNSET_TRANSITION_TIME_SECONDS / 2);
  
  if (current_time < sunrise_transition_start) {
    Serial.print("Sun's set (AM)\n");
    return NIGHT_COLOR;
  } else if (current_time >= sunrise_transition_start && current_time < sunrise_transition_end) {
    // RGB of (2000K + (seconds since transition start))
    uint16_t temperature = 1000 + (current_time - sunrise_transition_start);
    
    sprintf(serial_buffer, "Sun's coming out. Temperature %u", temperature);
    Serial.println(serial_buffer);
    return colorTemperatureToRGB(temperature);
  } else if (current_time >= sunrise_transition_end && current_time < sunset_transition_start) {
    uint16_t temperature = 4600;
    sprintf(serial_buffer, "Sun's out. Temperature %u", temperature);
    Serial.println(serial_buffer);
    // TODO: Constant
    return colorTemperatureToRGB(temperature);
  } else if (current_time >= sunset_transition_start && current_time < sunset_transition_end) {
    // RGB of (5600K - (seconds since transition start))
    uint16_t temperature = 4600 - (current_time - sunset_transition_start);

    sprintf(serial_buffer, "Sun's setting. Temperature %u", temperature);
    Serial.println(serial_buffer);
    return colorTemperatureToRGB(temperature);
  } else if (current_time >= sunset_transition_end) {
    Serial.print("Sun's set (PM)\n");
    return NIGHT_COLOR;
  }

  // error scenario
  Serial.print("Aw crap\n");
  return NIGHT_COLOR;
}

void real_loop() {
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

void fake_loop() {
  static time_t current_time_t = 1522587600; // shortly before sunrise
  DateTime current_time = DateTime(current_time_t);
  
  sprintf(serial_buffer, "It's %02u:%02u:%02u", current_time.hour(), current_time.minute(), current_time.second());
  Serial.println(serial_buffer);
  CRGB sun_color = getSunColor(current_time);
  fill_solid(leds, NUM_LEDS, sun_color);
  FastLED.show();
  delay(1000);
  current_time_t += 60; // a minute
}

void loop() {
  #ifdef DEBUG
  fake_loop();
  #else
  real_loop();
  #endif
}


//char serial_buffer[128] = "";
//sprintf(serial_buffer, "Sunrise over there is %lu (UTC)", sunrise_time);
//Serial.println(serial_buffer);
//sprintf(serial_buffer, "(%lu, local time)", sunrise_time + UTC_OFFSET_HOURS * ONE_HOUR);
//Serial.println(serial_buffer);
//sprintf(serial_buffer, "Sunset over there is %lu", sunset_time);
//Serial.println(serial_buffer);

