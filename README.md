# Basic text-based console menu system for AT91SAM9263 microcontroller
Solution of assignment for Embedded Systems course.


Assignment:
Write a simple program for an embedded system that implements the hierarchical menu on the serial console. The program should fulfil the following requirements:

Messages are read and written on the minicom terminal on PC,
Program should run on ARM processor connected to PC using EIA RS 232 serial interface (DGBU module) with the following parameters: baudrate 19200 bauds, 8 data bits, no parity, single stop bit (configuration should be parameterised in a separate config.h file), no FIFO buffer required,
Write drivers for LEDs, pushbuttons and DGBU serial interface with suitable functions required for each device (notice that not all functions are required for drivers), 
After reset the program should welcome the user with the following message and should start from the top level of menu (‘>’ should be displayed on each level):

“ Simple menu by Name Surname

                Write ‘help’ to obtain more information

                 >”
User can change level after the suitable command (e.g. LED, Button or DBGU),
User can return back to  the higher level using command ‘up’,
User can turn on/off LEDs  using com. on the ‘LED‘ level (e.g. SetLED A or ClearLED B),
User can read status of buttons using commands on the ‘Button’ level (e.g. ReadButton B),
User can read configuration of DBGU serial port using commands on the ‘DGBU’ level (e.g. DevicesStatus),
All commands should be case insensitive (both commands are valid: SetLED, setled)
Help command display help on current level only for current device
Menu structure:
Menu level

> 

 LED>         LED

          SetLED

          ClearLED

          BlinkLED

          LEDstatus

          ChangeLED

 

 Button>     Button

          ReadButton

          PullupEn

          PullupDis

 

  DBGU>     DBGU

          DeviceStatus

Command function

Top level
Turn on LED (A or  B)
Turn off LED (A or  B)
Blink LEDs five times
Display status of LED A or LED B, according to the parameter
Invert LED (A or B)

Read status of button A or B
Enable pull-up
Disable pull-up

Display baudrate, number of databits and parity bits, informations read from registers and then calculated baudrate

Example of commands:
settled A
SetLed A
SetLED A
SETLED A
ledstatus B
changeled B
readbutton B
pullupdis A
device status
