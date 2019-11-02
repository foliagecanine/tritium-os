#include <kernel/time.h>

#define CURRENT_YEAR        2019                          // Change this each year!
#define STARTING_YEAR      2000                           //Change this if you want...
 
#define RTC_Second	0
#define RTC_Minute	2
#define RTC_Hour	4
#define RTC_Day		7
#define RTC_Month	8
#define RTC_Year	9 

int century_register = 0x00;                                // Set by ACPI table parsing code if possible
 
uint8_t second;
uint8_t minute;
uint8_t hour;
uint8_t day;
uint8_t month;
uint32_t year;
int time_zone_modifier;

 
void set_time_zone(int tz) {
	time_zone_modifier = tz;
}

int get_time_zone() {
	return time_zone_modifier;
}
 
void read_rtc();

uint8_t bcdToBinary(uint32_t bcd) {
  uint8_t out = 0;
  out+=((bcd>>4)*10);
  out+=bcd&0x0F;
  return out;
}

// these are the values of the rtc NOT THE ADJUSTED TIME
uint32_t current_year_rtc() { 
	read_rtc();
	return year;
}

uint8_t current_month_rtc() {
	read_rtc();
	return month;
}

uint8_t current_day_rtc() {
	read_rtc();
	return day;
}

uint8_t current_hour_rtc() {
	read_rtc();
	return hour;
}

uint8_t current_minute_rtc() {
	read_rtc();
	return minute;
}

uint8_t current_second_rtc() {
	read_rtc();
	return second;
}
 
//this won't be used except internally. it is used for getting the time zone data, because it is so so so much harder to adjust them one at a time
//this is required because if say it were 23:00 12/31/2019 GMT and someone lived at GMT+1, it would change all the values: 0:00 1/1/2020 GMT+1 <happy new year!>
//that way, we calculate the hour as a uint32_t, and recalculate the day/month/year from the hour.
uint32_t full_hour_time() {
	//days in previous months (first set is non-leap-year, second set is leap year) <I hope I got the math right ;-) >
	const int days_in_month[] = {0,31,59,90,120,151,181,212,243,273,304,334,  0,31,60,91,121,152,182,213,244,274,305,335};
	
	uint32_t full_hour = 0;
	full_hour += hour;
	full_hour += time_zone_modifier;
	full_hour += (day)*24;
	//this one will take some explaining... it deals with adding previous months to the hour, but has to deal with leap years too.
	//leap years occur every four years except when the year is divisible by 100 but not by 400. explanation will be in angle brackets: <>
	//so therefore if year%4==0<if is divisible by 4>&&(year%400==0<and either it's divisible by 400>||year%100!=0<or it is not divisible by 100 at all>)
	//then use leap year (add 12 to days_in_month and times by 24) otherwise use non-leap-year (just month) and times by 24
	//The hours start at year STARTING_YEAR
	full_hour += ((year-STARTING_YEAR)%4==0&&((year-STARTING_YEAR)%400==0||(year-STARTING_YEAR)%100!=0) ? days_in_month[month+11]*24 : days_in_month[month-1]*24);
	
	full_hour += (
          ((year-STARTING_YEAR)*365) //days in a year
          +((year-STARTING_YEAR)/4) //if its a leap year add one per year
          +(year-STARTING_YEAR>0 ? 1 : 0) //don't worry about STARTING_YEAR, that's already taken care of
          -((year-STARTING_YEAR)/100) //if it is divisible by 100 it is not a leap year, so subtract one per year
          +((year-STARTING_YEAR)/400) //unless it happens to be divisible by 400 so add one per year
        )*24; //multiply every day by 24

    return full_hour;
}

uint32_t current_hour_time() {
	//read_rtc();
	return (unsigned char)(full_hour_time()%24);
}

uint32_t current_year_time() {
	//read_rtc();
	int curr_year = 0;
	uint32_t hours_remaining = full_hour_time();
  while(true) {
    if (curr_year%4==0&&(curr_year%100!=0||curr_year%400==0)) {
      if (hours_remaining >= 8784) {
        hours_remaining-=8784;
        curr_year++;
      } else {
        break;
      }
    } else {
      if (hours_remaining >= 8760) {
        hours_remaining-=8760;
        curr_year++;
      } else {
        break;
      }
    }
  }
  return curr_year+STARTING_YEAR;
}

uint32_t current_month_time() {
  const int days_in_month[] = {31,59,90,120,151,181,212,243,273,304,334,365,  31,60,91,121,152,182,213,244,274,305,335,366};
  uint32_t hours_remaining = full_hour_time();
  int curr_year = 0;
  while(true) {
    if (year%4==0&&(year%100!=0||year%400==0)) {
      if (hours_remaining >= 8784) {
        hours_remaining-=8784;
	curr_year++;
      } else {
        uint32_t last_month = 0;
        for (int i = 0; i < 12; i++){
		printf(" %ld",hours_remaining);
          if (!(hours_remaining>(days_in_month[i+12]*24))) {
            printf("(%d)",i);
            return (i+2)==13 ? 1 : (i+2);
          }
        }
      }
    } else {
      if (hours_remaining >= 8760) {
        hours_remaining-=8760;
	curr_year++;
      } else {
        for (int i = 0; i < 12; i++){
          if (!(hours_remaining>(days_in_month[i]*24))) {
            return i+1;
          }
        }
      }
    }
  }
}

uint32_t current_day_time() {
  //read_rtc();
  const int days_in_month[] = {0,31,59,90,120,151,181,212,243,273,304,334,  0,31,60,91,121,152,182,213,244,274,305,335};
  int curr_year = 0;
  uint32_t hours_remaining = full_hour_time();
  while(true) {
    if (curr_year%4==0&&(curr_year%100!=0||curr_year%400==0)) {
      if (hours_remaining >= 8784) {
        hours_remaining-=8784;
        curr_year++;
      } else {
	hours_remaining-=days_in_month[current_month_time()+11]*24;
        break;
      }
    } else {
      if (hours_remaining >= 8760) {
        hours_remaining-=8760;
        curr_year++;
      } else {
	hours_remaining-=days_in_month[current_month_time()-1]*24;
        break;
      }
    }
  }
  return hours_remaining/24;
}

uint32_t current_minute_time() {
	return current_minute_rtc();
}

uint32_t current_second_time() {
	return current_second_rtc();
}
 
int get_value_rtc(int val) {
	switch(val) {
		case 0:
			return (int)second;
			break;
		case 1:
			return (int)minute;
			break;
		case 2:
			return (int)hour;
			break;
		case 3:
			return (int)day;
			break;
		case 4:
			return (int)month;
			break;
		case 5:
			return (int)year;
			break;
		default:
			return 0;
			break;
	}
}
 
void read_rtc() {
  outb(0x70,10);
  while (inb(0x71)&0x80)
    ;
  
  outb(0x70,RTC_Second);
  second = bcdToBinary(inb(0x71));
  
  outb(0x70,RTC_Minute);
  minute = bcdToBinary(inb(0x71));
  
  outb(0x70,RTC_Hour);
  hour = bcdToBinary(inb(0x71));
  
  outb(0x70,RTC_Day);
  day = bcdToBinary(inb(0x71));
  
  outb(0x70,RTC_Month);
  month = bcdToBinary(inb(0x71));
  
  outb(0x70,RTC_Year);
  year = bcdToBinary(inb(0x71));
  year+=STARTING_YEAR;
}
