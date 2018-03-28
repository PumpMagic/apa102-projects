#include <time.h>
#include <Wire.h>
#include "RTClib.h"

#define LATITUDE 37.0
#define LONGITUDE -122.0
#define UTC_OFFSET_HOURS -7

// What time we reset the RTC to in the event that it's lost its power
#define RTC_RESET_YEAR    2017
#define RTC_RESET_MONTH   1
#define RTC_RESET_DAY     1
#define RTC_RESET_HOUR    0
#define RTC_RESET_MINUTE  0
#define RTC_RESET_SECOND  0

// time.h docs: https://www.nongnu.org/avr-libc/user-manual/group__avr__time.html
// TimeLord example: https://github.com/probonopd/TimeLord/blob/master/examples/SunriseSunset/SunriseSunset.ino
// time.h avr history: http://swfltek.com/arduino/timelord.html

static RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);

  Serial.println("Hi!\n");

  rtc.begin();

  // If the RTC's lost power, reset it
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(RTC_RESET_YEAR, RTC_RESET_MONTH, RTC_RESET_DAY,
                        RTC_RESET_HOUR, RTC_RESET_MINUTE, RTC_RESET_SECOND));
  }


  // Get today's sunrise time
  // First, get the time from the RTC
  DateTime current_time = rtc.now();

  // Round the current time to noon in a time.h-friendly struct, to get the sun times
  struct tm current_time_tm;

  current_time_tm.tm_isdst = 0; // our calculations expect there to be no DST; we're working with the sun
  current_time_tm.tm_yday = -1; // time library's sun functions ignore this
  current_time_tm.tm_wday = -1; // time library's sun functions ignore this
  current_time_tm.tm_year = current_time.year() - 1900; // datetime returns 0-based, tm_year is 1900-based
  current_time_tm.tm_mon = current_time.month() - 1; // datetime is 1-based, tm_mon is 0-based
  current_time_tm.tm_mday = current_time.day(); // datetime and tm_mday are both 1-based
  current_time_tm.tm_hour = 12;
  current_time_tm.tm_min = 0;
  current_time_tm.tm_sec = 0;

  // Hardcoded - for testing without RTC
//  current_time_tm.tm_year = 118;
//  current_time_tm.tm_mon = 2;
//  current_time_tm.tm_mday = 25;
//  current_time_tm.tm_hour = 12;

  set_position(LATITUDE * ONE_DEGREE, LONGITUDE * ONE_DEGREE);
  // set_zone doesn't seem to affect the output of sun_rise() or sun_set(), so don't bother
//  set_zone(UTC_OFFSET_HOURS * ONE_HOUR);

  // The sunrise and sunset times seem to depend on the hour, oddly enough; not just the day.
  // Only by about 90 seconds, though.
  // Just pass in noon, as the TimeLord example suggests.
  time_t time_now = mktime(&current_time_tm);
  time_t sunrise_time = sun_rise(&time_now);
  time_t sunset_time = sun_set(&time_now);
  char serial_buffer[128] = "";
  sprintf(serial_buffer, "Sunrise over there is %lu (UTC)", sunrise_time);
  Serial.println(serial_buffer);
  sprintf(serial_buffer, "(%lu, local time)", sunrise_time + UTC_OFFSET_HOURS * ONE_HOUR);
  Serial.println(serial_buffer);
  sprintf(serial_buffer, "Sunset over there is %lu", sunset_time);
  Serial.println(serial_buffer);
}

void loop() {
  // State machine:
  //  1. Nighttime: we're in this state when the time is in the range [time.h's sunset plus delta, time.h's sunrise minus delta).
  //  2. Sunrise: when the time is in the range [time.h's sunrise minus delta, time.h's sunrise plus delta).
  //      lights are in the range (off, max daylight color) depending on the time.
  //  3. Daytime:when the time is in the range [time.h's sunrise plus delta, time.h's sunset minus delta).
  //  4. Sunset: lights transitioning from max daylight color to off. State in [time.h's sunset minus delta, time.h's sunset plus delta).
  // State transitions are 1 -> 2 -> 3 -> 4. Maybe near the poles we'd need a different state machine. Assume we aren't, for simplicity.
  // "Color", above, refers to the LEDs' hue, saturation, and value.
  // Go from about 2000K to 5500K. Try starting with (255, 147, 41) to (255, 255, 251).
}
