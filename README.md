# EspLight-firmware
firmware for on a EspLight Board.

# first time use.
the first time you use the EspLight there will probably be no firmware it 

if no firmware follow 'loading precompiled firmware'

first time use with firmware:
* press the AP button to turn the EspLight into a access point.
* connect with the EspLight via the wifi. the first time it'll have a ssid like 
EspLight-xx where xx are numbers.
* surf to the configuration page. (type 192.168.4.1 and hit enter)
* in the configuration page, you'll have two options. Wifi and Led.
* fill in your wifi settings on the Wifi page and hit save.
* then in the LED page set the kind of leds you have and how many. in the case of 
analog leds you don't have to enter a number.

# loading precompiled firmware.
to load the precompiled firmware you'll have to download and install esptool:

https://github.com/themadinventor/esptool

next you'll have to connect your esplight to a usbserial converter.
* how to hook one up that came with the EspLight: https://www.tkkrlab.nl/wiki/EspLight#Program_the_firmware_into_the_EspLight

you can find the binary in the bin folder.

next you run :

```
$ esptool.py write_flash -ff 80m -fm qio -fs 32m-c1 0x00000 firmware.bin
```

after uploading you can follow the instructions for first time use.
