# tst_tpms
Receive and decode TST-507 TPMS signals.

## background

The TST (Truck Systems Technologies) 507 Series TPMS is an aftermarket TPMS system for trucks & RV's.  It uses 2 types of valve stem screw-on sensors, one a "cap" type and the other a "flow-thru" type.  The sensors are read via a small dash mounted receiver. 

(https://tsttruck.com/shop/kit-sales-us.html)

Specifications:
- Temp range = 40F-176F / -40C-80C
- Pres range = 0-198psi / 0-13.5bar
- Temp accuracy = +/- 3F
- Pres accuracy = +/- 3psi, 0.2bar

The system uses a radio frquency of 433.92Mhz.  Each sensor has a 6 character ID, known to the user and needed to setup the receiver.  Sensors transmit at varying intervals, at least every 5 minutes, or when measurements change, and do transmit when not moving.

## implementation

This project uses RadioLib (https://github.com/jgromes/RadioLib)

Heltec WiFi LoRa 32 (V2) modules were used initially because of their large bang for the buck.  Each module has ESP32 + SX127x + WiFi + BLE + LiPo management + on board display.  the SX1278 is capable of LoRa, FSK, and OOK.  We need OOK for this TPMS system.  This is an Arduino based module.

## to do
Get running on other modules such as CC1101
