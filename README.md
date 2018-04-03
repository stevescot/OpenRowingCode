# OpenRowingCode
Tools for rowing (both ergometers, and 'real' rowing) 

ArduinoRowComputer - contains a sketch which will run on an Arduino Uno, Or Nano , and can hook directly to the jack of a Concept 2 - giving the stats you would normally see on a PM3. With an LCD shield on an UNO - you also get a menu, with Just Row, Intervals, Watts, Drag factor etc.. 

To work with a concept 2 - connect one side of the sensor to GND and the other to A1 - the sketch on startup should print out:

"Concept 2 detected on pin 1"

Then Strokes will be printed to serial in tab delimited form.

With an LCD Shield, Split, SPM, total Distance, power graph and total time are displayed.

ArduinoRowComputeresp - is a sketch for an ESP8266 chip to do the same thing, but to upload to a webserver so that you can use your mobile phone as a display, and so that GYMs, and rowing clubs etc - can display multiple people rowing at once
(still a work in progress)

[https://www.instructables.com/id/ARDUINO-MONITOR-FOR-CONCEPT2-MODEL-B-C-D/]
[https://www.youtube.com/watch?v=9e-qV-pyVDc&feature=youtu.be]
[https://www.youtube.com/watch?v=0bPW8SFKenU]
