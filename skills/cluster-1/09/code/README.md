# Code Readme

Please describe what is in your code folder and subfolders. Make it
easy for us to navigate this space.

Also
- Please provide your name and date in any code submitted
- Indicate attributrion for any code you have adopted from elsewhere


This code was adopted from the mcpwm servo control example from the esp-idf github.
We used this code since it had all the configurations set up for the servo that we 
use in this skill. To wire up the servo, we connected it to ground, power and GPIO
pin 14. As we were testing the servo, we realized that changing the SERVO_MIN_PULSEWIDTH
and the SERVO_MAX_PULSEWIDTH would changed the angle of rotation for the servo. When
the two pulsewidths were close together, we noticed that it would only rotate a small
amount so we decided to push the bounds for the pulsewidths and my servo was able to
turn the full 180 degrees with a min of 530 and a max of 2400.
The main code is in the main folder called mcpwm_servo_control_example.c