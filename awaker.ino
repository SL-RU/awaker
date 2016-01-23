#include "leOS2.h" //include the scheduler
#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>

#define fst_c_pin 4
#define lst_c_pin 10
#define row_pin1  11
#define row_pin2  12
#define spk_pin   3

//Analog pins
#define bset_clock 7     //4
#define bset_alarm 2     //1
#define bdelay_alarm 1   //2
#define bcancel_alarm 0  //3
#define button_press_time 50

unsigned int notes[] = {31, 46, 73, 94, 128, 163, 397, 501, 712, 953, 1070, 1431, 1607, 2148};

leOS2 myOS; //create a new istance
RTC_DS1307 rtc;

byte mode = 0; //0 - clock
			   //1 - set alarm
			   //2 - set clock
			   //


//This values are displaying in leds
byte _minutes = 0;
byte _hours = 0;
byte _seconds = 0;
byte _alarm_led = 0;
byte _clock_led = 0;
byte _alarm_sound = 0;

//This is current date and time
byte minutes = 0;
byte hours = 0;
byte seconds = 0;
byte date = 0;

//this is alarm time and after what date alarm must activate
byte alarm_on = 0;
byte alarm_minutes = 0;
byte alarm_hours = 0;
byte alarm_date = 0;

void setup() {
	Serial.begin(9600);
	myOS.begin(); //initialize the scheduler

	if (! rtc.begin()) {
    	Serial.println("Couldn't find RTC");
    	while (1);
  	}

	if (! rtc.isrunning()) {
	  	Serial.println("RTC is NOT running!");
	  	// following line sets the RTC to the date & time this sketch was compiled
	  	rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	  	// This line sets the RTC with an explicit date & time, for example to set
	  	// January 21, 2014 at 3am you would call:
	  	// rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
	}

	for(int i = fst_c_pin; i<=lst_c_pin; i++)
		pinMode(i, OUTPUT);	
	pinMode(row_pin1, OUTPUT);	
	pinMode(row_pin2, OUTPUT);	
	pinMode(spk_pin, OUTPUT);


	myOS.addTask(display, myOS.convertMs(7));
	myOS.addTask(control, myOS.convertMs(21));

	alarm_on = EEPROM.read(0);
	alarm_minutes = EEPROM.read(1);
	alarm_hours = EEPROM.read(2);
	alarm_date = EEPROM.read(3);
}

float sinVal;
int toneVal;
int i;

void loop() 
{
	DateTime now = rtc.now();
	seconds = now.second();
	minutes = now.minute();
	hours = now.hour();
	date = now.day();
	
	switch (mode) {
	    case 0:
	    	update_clock();
	    break;
	    case 1:
	    	update_set_alarm();
	    break;
	    case 2:
		    update_set_clock();
	    break;
	}

	delay(200);
	if(_alarm_sound)
		tone(3, notes[random(0, 15)]);
	else
		noTone(3);
}

void update_clock()
{
	_minutes = minutes;
	_seconds = seconds;
	_hours = hours;
	_alarm_led = alarm_on;
	_clock_led = 0;
}
void update_set_alarm()
{
	if (_alarm_led) _alarm_led = 0; else _alarm_led = 1;
}
void update_set_clock()
{
	if (_clock_led) _clock_led = 0; else _clock_led = 1;
}

void check_alarm()
{

}

void display()
{
	digitalWrite(row_pin1, 0);
    digitalWrite(row_pin2, 1);
   	for(i = fst_c_pin; i<=lst_c_pin; i++)
	{
		digitalWrite(i, 0);
		if (i - fst_c_pin < 6)
		{
			if((_minutes >> (i - fst_c_pin)) & 0b00000001)
			{
				digitalWrite(i, 1);
			}
		}
		else
		{
			if(i - fst_c_pin == 6 && _seconds % 2 == 1)
				digitalWrite(i, 1);
		}
	}

	delay(1);
	digitalWrite(fst_c_pin + 6, 0);

	delay(9);

	digitalWrite(row_pin1, 1);
	digitalWrite(row_pin2, 0);
	for(i = fst_c_pin; i<=lst_c_pin; i++)
	{
		digitalWrite(i, 0);
		if (i - fst_c_pin < 4)
		{
			if(_hours > 12)
				if(((_hours - 12) >> (i - fst_c_pin)) & 0b00000001)
				{
					digitalWrite(i, 1);
				}
			else
				if(((_hours) >> (i - fst_c_pin)) & 0b00000001)
				{
					digitalWrite(i, 1);
				}
		}
		else
		{
			if(i - fst_c_pin == 4 && _hours > 12)
				digitalWrite(i, 1);
			else if(i - fst_c_pin == 5 && _alarm_led)
				digitalWrite(i, 1);
			else if(i - fst_c_pin == 6 && _clock_led)
				digitalWrite(i, 1);
		}
	}

	delay(1);
	digitalWrite(fst_c_pin + 6, 0);
	digitalWrite(fst_c_pin + 5, 0);
	delay(2);
	digitalWrite(fst_c_pin + 4, 0);
}

byte _last_button = 0;
unsigned long _last_button_time = 0;
void control()
{
	byte button = 100;
	if(911 <= analogRead(bset_alarm))   {  button = bset_alarm;   }
	if(911 <= analogRead(bdelay_alarm)) {  button = bdelay_alarm; }
	if(911 <= analogRead(bcancel_alarm)){  button = bcancel_alarm;}
	if(911 <= analogRead(bset_clock))   {  button = bset_clock;   }

	//Serial.println(analogRead(2));
	if(button == 100)
	{
		if(_last_button != 100 && millis() - _last_button_time >= button_press_time && _last_button_time != 0)
		{
	Serial.println(_last_button);
			switch (mode) {
			    case 0:
			      control_clock(_last_button);
			      break;
			    case 1:
			      control_set_clock(_last_button);
			      break;
			    case 2:
			      control_set_alarm(_last_button);
			      break;
			}
		}
		_last_button_time =  0;
		_last_button = 100;
	}
	else
	{
		if(_last_button != button)
		{
			_last_button = button;
			_last_button_time = millis();
		}
	}

}

void control_clock(byte button)
{
	switch (button) {
	    case bset_alarm:
	      mode = 2;
	      break;
	    case bdelay_alarm:
	      // do something
	      break;
	    case bcancel_alarm:
	      alarm_on = 0;
	      EEPROM.write(0, alarm_on);
	      break;
	    case bset_clock:
	      mode = 1;
	      break;
	}
}
void control_set_clock(byte button)
{
	switch (button) {
	    case bset_alarm:
	      // do something
	      break;
	    case bdelay_alarm:
	      _minutes --;
	      break;
	    case bcancel_alarm:
	      _minutes ++;
	      break;
	    case bset_clock:
	      mode = 0;
	      break;
	}
}
void control_set_alarm(byte button)
{
	switch (button) {
	    case bset_alarm:
	      mode = 0;
	      break;
	    case bdelay_alarm:
	      _alarm_sound = 1;
	      break;
	    case bcancel_alarm:
	      _alarm_sound = 0;
	      break;
	    case bset_clock:
	      // do something
	      break;
	}
}