# A2000
Basic library for ESP32 that can setup communications with A2000 power meter over RS485 (or RS232). All three channels are readout and placed inside a struct that the user can access.

13/10/2022: This only works on old version of the ESP IDF (1.0.6), the new version of the IDF (2.x.x) give problems as the serial routines have changed (custom DTR driving).

A second version for the Arduino UNO has been added
