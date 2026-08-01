# Morning_Clock
An alarm clock with dawn effect based on AVR ATmega8 MCU with RTC

![Morning Clock](https://github.com/SignallerK/Morning_Clock/blob/master/Photos/Main_photo.png)
## Description
This a simle alarm clock with dawn effect based on AVR ATmega8.
## Features
* It uses RTC on its asyncronic timer only external quartz 32768Hz is necessary.
* Gamma corrected (logarithmicall) LED PWM control.
* Supports any light sourse via USB output up to 1A (depends on transistor). **Warning** there is no current limitation, if R3 is not installed.
* Alarm can be turned on/off for each day of week.
* Selectable dawn length.
* No annoying beeper ;)
* All constant settings are stored in EEPROM
* In case of power failure, clock will continue to operate from battery (3xAAA)
## Menu Modes/Manual
Modes are given in switching order, and are changed by pressing **Set** button.
* Clock - **default mode** - shows current time. Second dot (HH.MM) is blinking in all modes with frequency 0.5Hz indicating that clock is working. Buttons operations:
  * **H button** - press to increase value of hours.
  * **M/D button** - press to increase value of minutes.
  * **H + M/D button for small time** - press to turn on or off alarm clock globally. On - 4th dot is lit (HHMM.), off - 4th dot is not lit.
* Alarm Сlock- shows alarm time. This mode is indicated by first dot (H.HMM). Alarm Clock time is the dawn end time. For example, if dawn perion is half an hour, and alarm is set to 7:30, then dawn will start at 7:00. Buttons operations:
  * **H button** - press to increase value of alarm hours.
  * **M/D button** - press to increase value of alarm minutes.
* DAY_SET - **ds 5** - setting day of the week 1-7.
  * **M/D button** - press to set day.
* Al_Mon - **dA 1** -  turns on/off alarm for 1st day of the week.
  * **M/D button** - press to turn on alarm (4th dot is lit (HHMM.)  or off ( 4th dot is not lit.).
*	Al_Tue -**dA 2** - turns on/off alarm for 2st day of the week. Works similar to Al_Mon.
*	Al_Wed -**dA 3**.
*	Al_Thu -**dA 4**.
*	Al_Fri -**dA 5**.
*	Al_Sut -**dA 6**.
*	Al_Sun -**dA 7**
* Bright -**br 1** - set display brightness.
  * **M/D button** - press to set brightness - 2 bright, 1- dim.
* Indication - **In 0** - indication period. 0 - clock diplays data only 60 sec, then only second and 4th dot is lit. 1- constant light.
  * **M/D button** - press to set Indication - 1 constant, 0- 60 second.
  * **H button or M/D button** - shot press to turn on display.
* Dawn Period -**dP25** - dawn period - from 15 to 30 minutes with step 5 minutes.
   * **M/D button** - press to set dawn period
* Time Correction - **C 00** - daily time correction from -15 to +15 seconds (step 1 sec). Automatically applied once a day at 03:00:00.
   * **M/D button** - press to cycle correction value (-15 to +15).
### Global buttons operations
* When alarm is working (light is turned on) is can be switched off by short press of **H button or M/D button**.
* Press **H button or M/D button** to turn on display when indication mode 0 is selected.
## TBD (probably will be added):
* Add speaker and make beeps when alarm time is on (**!**  Is not supported by current pcb);
