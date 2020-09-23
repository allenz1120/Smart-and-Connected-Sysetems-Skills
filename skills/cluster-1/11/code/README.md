# Code Readme

This code was adopted from the timer example from the code examples github that was 
provided to us. We used this code as a starting point since it had configuration set
up already for a pulsing timer using ESP32 timer functions. However, instead of
displaying Action! at each second, we developed code to display a timer on the board.

We did this by allocating a global counter that would increment during each cycle 
that evt.flag was equal to 1. Then, we moved the the button interrupt code from the 
last skill into this skill. However, instead of having just one flag again, we 
implemented a resetFlag and a startFlag to signal when to start the counter and when 
reset it back to 00. 

The main code is in the main folder and is called timer.example.c
