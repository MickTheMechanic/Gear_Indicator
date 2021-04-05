# Gear_Indicator
## An open-source arduino based gear indicator with shift light

<a rel="license" href="http://creativecommons.org/licenses/by-nc/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc/4.0/">Creative Commons Attribution-NonCommercial 4.0 International License</a>.
#

[<img src="https://i.ibb.co/6g25gTd/Screenshot-16.png" alt="vid" border="0">](https://youtu.be/Q-SuN3IAmyA)


#

## Overview

# gear indicator
Gear lever position is measured by 2 potentiometers (in the same manner as a joystick). Current gear is determined by X and Y position of the gear lever.
A 16 segment LED display is controlled via a 16 bit shift register, which accepts serial data from the Arduino

# shift-light
A voltage divider and transistor convert a 12v square wave tacho signal to an acceptable input signal for the Arduino. 
A row of NeoPixel LED's are then illuminated according to the tacho signal

#

<a href="https://ibb.co/n0skRL3"><img src="https://i.ibb.co/MhDN9C8/IMG-1073.png" alt="IMG-1073" border="0"></a>
<a href="https://ibb.co/vxfMc01"><img src="https://i.ibb.co/ynKxNH0/Screenshot-17.png" alt="Screenshot-17" border="0"></a>
