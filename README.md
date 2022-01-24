# tst_tpms
Receive and decode TST-507 TPMS signals.

## Background

The TST (Truck Systems Technologies) 507 Series TPMS is an aftermarket TPMS system for trucks & RV's.  It uses 2 types of valve stem screw-on sensors, one a "cap" type and the other a "flow-thru" type.  The sensors are read via a small dash mounted receiver. 

(https://tsttruck.com/shop/kit-sales-us.html)

Specifications:
- Temp range = 40F-176F / -40C-80C
- Pres range = 0-198psi / 0-13.5bar
- Temp accuracy = +/- 3F
- Pres accuracy = +/- 3psi, 0.2bar

The system uses a radio frquency of 433.92Mhz.  Each sensor has a 6 character ID, known to the user and needed to setup the receiver.  Sensors transmit at varying intervals, at least every 5 minutes, or when measurements change, and do transmit when not moving.

## Implementation

This project uses RadioLib (https://github.com/jgromes/RadioLib)

Heltec WiFi LoRa 32 (V2) modules were used initially because of their large bang for the buck.  Each module has ESP32 + SX127x + WiFi + BLE + LiPo management + on board display.  the SX1278 is capable of LoRa, FSK, and OOK.  We need OOK for this TPMS system.  This is an Arduino based module.

## TST-507 basics

The sensors transmit data periodically on 443.92Mhz using OOK modulation.  The 8 byte data packet contains a checksum, sensor ID, and measurement data.

The data packet looks like this:

URH analog view:
![analog](https://user-images.githubusercontent.com/20508816/150719833-2e1b397e-f9be-414d-bd7e-c03a8261e8df.png)

URH demodulated view:
![demodulated](https://user-images.githubusercontent.com/20508816/150719981-64000301-30e4-42b5-baad-9f7d474f099f.png)

URH NRZ bits:
![bits_nrz](https://user-images.githubusercontent.com/20508816/150720036-d1e0f646-27f6-4242-8e88-ae15a5b69ebf.png)

URH Manchester I hex:
![hex_manchester_i](https://user-images.githubusercontent.com/20508816/150720065-10196355-b948-48df-8451-88ce38192a26.png)

From this we can see:
- 32 bits of preamble (1010...)
- checksum of 0xE5 (byte 0)
- sensor ID of 0x563ED9 (bytes 1-3)
- data of 0x2C4B0001

Decoding the data portion we get:
- pressure = 2nd nibble of byte 7 + byte 4 = 0x012C
  - 0x012C / 0.4 = 750 kPa
  - 750 / 6.895 = 108.8 psi
- temperature = byte 5 = 0x4B
  - 0x48 - 50 = 25 deg C
  - (25 * 1.8) + 32 = 77 deg F
- checksum = (sum of bytes 1-7) % 256

## Modules

### Heltec WiFi LoRa 32 (V2)

This module has packet mode or direct mode. Decodes Manchester II natively.

## To Do
Get running on other modules such as CC1101
