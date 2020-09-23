# Code Readme

This code was adopted from the uart task example from the esp-idf github. We did this
because it has the skeleton for multiple tasks already. From this skeleton, we created
3 different tasks. One for the LED binary blinking, another for the alphanumeric
display and one for the button interrupt. 

We started this skill by copying over the code we wrote in skill 7 which was the binary
LED blink task. This gave us a good starting point since we tested the tasks using 
our existing working code. We then added a flag that declared the direction of 
counting with the LEDs. This flag is triggered by the button press and when it has the value
of 0, it is counting up and counts down when it has the value of 1.

Once the blinking was confirmed working in one direction, we started on the button task
by wiring the button in to power and the GPIO pin 26. This pin was then given an 
interrupt task and it would invert the flag each time that it was pressed.

Lastly, once the LEDs were working and the button was able to reverse the direction
of counting, the next step was the set up the alphanumeric display. We copied over 
most of the code from skill 8 since it had just about everything we needed for the task.
We merged the two init functions into one and moved the rest of the functions necessary
for the dispaly. Then, using the flag, we were able to change the display to show "UP"
or "DOWN".

The main code is in the main folder and is called uart_async_rxtxtasks_main.c
