BFC
===

Description
___________

The Binary Function Computer is  a bytebeat-based oscillator for the modular synth. It was heavily influenced by information found on:

http://countercomplex.blogspot.com/2011/10/algorithmic-symphonies-from-one-line-of.html

Based on the formerly defined binary logic manipulation, toggles equation constants and conditional variation of equations by analysing modular logic signals. It will have a comparator to derive logic-out signals based on the periodic maximum and arithmetic mean of the function values.

The main control of the unit should focus on manipulation and exploration of bytebeat equations, which can be pretty puzzling to figure out in their own. To do so, state change has to predictable, which is why i chose logic inputs over more complex CV-driven manipulation (modulated sine LFO signals for example). Also, i think there are too few interesting logic implementations for modular systems. Im hoping this project can contribute to some change of that.

The following thread on the muffwiggler forums will be used to announce updates:


http://www.muffwiggler.com/forum/viewtopic.php?t=95053#1321154

Furthermore, this text focuses on the theoretical part of defining writing functions with specific tone durations:

https://github.com/aempirei/Bit-Music/blob/master/README

The current version is tested on Ubuntu 12.04 LTS on an arduino UNO, in the following configuration:

Digital In:
9,10: 2xLED
11:PWM out -> speaker

If input 3 is connected to 3v3 volts, that input gets read as HIGH, which engages an algorithm that calculated threshold values, used by the comparator function to derive digital output signals from the PWM signal. These will, in future versions, be used for trigger and logic signals within the Eurorack system. The PWM output can be fed to the "hard sync" input of a VCO for interesting sequences.


Dependencies:
____________

Tested using ino (http://inotool.org/) for compiling and uploading the sketch to the Arduino. It requires 1.0 version of the Arduino IDE. After running:

git clone http://github.com/Vurma/BFC

in a new directory, simply enter the following in the command line:

ino build
ino upload
