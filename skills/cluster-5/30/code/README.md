# Code Readme

This code uses two main tasks: drive_control and steering_control. The drive_control task uses mcpwm_set_duty_in_us to alternate the car between moving forwards, backwards and idle. Similarly, the steering_control rotates the car's wheels to the right, center, and left every one second. To calibrate the car, we used documentation from whizzer for the calibrateESC code.
