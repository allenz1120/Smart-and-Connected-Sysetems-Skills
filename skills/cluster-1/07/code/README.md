# Code Readme

This skill was done by setting three pins on the ESP32 board to be output gpio pins
and modulating the power that they received. At the beginning of doing this task, I 
was attempting to do it by hard coding the leds to blink in a specific sequence but 
after talking with Sam Krasnoff in the class, he explained to me a way to better 
automate the process and not hard code it. I then used a counter to act as a place
holder for what number from 0-15 I am on and it increments on each cycle of the while(1)
loop. 
The main code is in the main folder as blink.c
