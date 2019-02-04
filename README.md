# M5STACKRFID
M5 Stack Rfid reader

Hardware:
M5Stack ESP32 Basic Core Development Kit
M5GO RFID base

Expanded version of the M5Stack RFID example to be able to dump a Mifare Classic 1k card to the SD card.

Normally the neopixels are off by default, but mine required me to turn them off. So the Adafruit Neopixel library is used in the project. If your stay off on their own then you can comment that out.

A SD card is also required as it will save it to the root directory with the UID in HEX and .mfd for the extension.

No cracking is actually being performed. It is using the the keys, you supply, in the default_key variable and iterates through them to find which ones are valid.

TODO:
-add additional feedback that a card is copied to the SD card audio(beeps) visual(neopixels)
-add in the ability to clone to another card
