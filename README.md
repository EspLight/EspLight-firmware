# EspLight-firmware
firmware for on a EspLight Board.

# loading precompiled firmware,
to load the precompiled firmware you'll have to download and install esptool:

https://github.com/themadinventor/esptool

next you do:

```
$ esptool.py write_flash 0x00000 webserver-esp.bin
```

you can find the binary in the bin folder.
