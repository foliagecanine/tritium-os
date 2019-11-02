#ifndef _KERNEL_TIME_H
#define _KERNEL_TIME_H

#include <kernel/stdio.h>
void set_time_zone(int tz);
int get_time_zone();
uint8_t current_second_rtc();
uint32_t current_second_time();
uint8_t current_minute_rtc();
uint32_t current_minute_time();
uint8_t current_hour_rtc();
uint32_t current_hour_time();
uint8_t current_day_rtc();
uint32_t current_day_time();
uint8_t current_month_rtc();
uint32_t current_month_time();
uint32_t current_year_rtc();
uint32_t current_year_time();
void read_rtc();

#endif
