# antuinov2.1
Improved antuino software

This is the software for Antuino, an RF lab in a box for radio hams, RF tinkerers. Both the circuit and the software are under GPL, feel free to use them. 

The Antuino is uses an Arduino Nano as its controller. The have to be compiled using the Arduino IDE that is downloadable from www.arduino.cc . Copy all these files into a folder first. It may prompt you to rename the folder, choose 'Yes'. 

IMPORTANT: 
The Antuino uses a modified version of the glcd library. This is included as a zip file in this repository. The Arduino has a simple way to handle libraries - Each library is a sub-folder inside the libraries folder. Download glcd.zip and extract it as 'glcd' folder inside your Arduino's library sub-folder. This will install it as a library in Arduino.
