# tst_tpms
Receive and decode TST-507 TPMS signals.

The TST (Truck Systems Technologies) 507 Series TPMS is an aftermarket TPMS system for trucks & RV's.  It uses 2 types of valve stem screw-on sensors, one a "cap" type and the other a "flow-thru" type.  The sensors are read via a small dash mounted receiver. 

[TST 507 Kits](https://tsttruck.com/shop/kit-sales-us.html)

Specifications:
- Temp range = 40F-176F / -40C-80C
- Pres range = 0-198psi / 0-13.5bar
- Temp accuracy = +/- 3F
- Pres accuracy = +/- 3psi, 0.2bar

The system uses a radio frquency of 433.92Mhz.  Each sensor has a 6 character ID, known to the user and needed to setup the receiver.  Sensors transmit at varying intervals, at least every 5 minutes, or when measurements change, and do transmit when not moving.

