#include <time.h>

#define LATITUDE 37.0
#define LONGITUDE -122.0
#define UTC_OFFSET_HOURS -7

void setup() {
  Serial.begin(9600);
  
  struct tm time_now_tm;

  time_now_tm.tm_isdst = 0; // no DST!!!
  time_now_tm.tm_yday = -1; // time library's sun functions ignore this
  time_now_tm.tm_wday = -1; // time library's sun functions ignore this
  
  // Eventually, with a DS3231...
//  DateTime now = rtc.now();
//  time_now.time_year = 
  
  time_now_tm.tm_year = 118;
  time_now_tm.tm_mon = 2;
  time_now_tm.tm_mday = 25;
  time_now_tm.tm_hour = 12;
  time_now_tm.tm_min = 0;
  time_now_tm.tm_sec = 0;

  set_position(LATITUDE * ONE_DEGREE, LONGITUDE * ONE_DEGREE);
  // set_zone doesn't seem to affect the output of sun_rise() or sun_set(), so don't bother
//  set_zone(UTC_OFFSET_HOURS * ONE_HOUR);


  Serial.println("Hi!\n");

  // The sunrise and sunset times seem to depend on the hour, oddly enough; not just the day.
  // Only by about 90 seconds, though.
  // Just pass in noon, as the TimeLord example suggests.
  time_t time_now = mktime(&time_now_tm);
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
  // put your main code here, to run repeatedly:

}
