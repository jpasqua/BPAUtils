# BPAUtils

A Collection of semi-random utility functions and classes.

## Contents

* BPABasics.h
	* A collection of constants, macros, and functions for things like unit conversion.
* ESP_FS.[h, cpp]
* GenericESP.[h, cpp]
	* Functions that mask the differences between ESP8266 and ESP32 system calls.
* HistoryBuffer.h, HistoryBuffers.h, Serializable.h
	* Keep track of a series of objects (often timestamped sets of sensor data) in a circular buffer. Provides the ability to load and store the data to a file in flash.
* Indicators.h
	* A mechanism for displaying a status on some sort of LED. It could be a single color LED or a multi-color one (like a NeoPixel) or something else by extending with new subclasses.
* MovingAverage.h
	* Keep track of a moving average of some value without storing all the values in the sequence.
* Output.[h, cpp]
	* Format data for output based on various parameters such as whether values should be displayed in metric or imperial units and whether times should be displayed in 24 hour format.


