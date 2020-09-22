# Code Readme

Please describe what is in your code folder and subfolders. Make it
easy for us to navigate this space.

Also
- Please provide your name and date in any code submitted
- Indicate attributrion for any code you have adopted from elsewhere


The code was modified using the Blink.c file as a starting base due 
to it's existing configuation for flashing the LED on the ESP board.It 
uses the uart driver for user I/O in the toggle mode of the program. This
allows the user to to press the "t" key and it will immediately toggle the
led on the ESP board without hitting enter. Once the "s" key is pressed, it
switches to the echo mode, the user will enter a word/phrase and once they 
have clicked enter, the monitor will echo whatever they have entered into 
the terminal. This is done using the gets function and we chose this since
we decided it was most logical to have it all echo out at once. If the user
presses the s key and enter, it switches to the hex mode, they will be able 
to enter any integer and the program will echo out the hex version of their 
input. We used a hex conversion code snippit from quora to accomplish this.


The main code file is in the folder main as blink.c
