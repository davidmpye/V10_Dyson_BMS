# V10_Dyson_BMS
A written-from-scratch unofficial firmware for Dyson V10 Battery Management Systems

Lots more documentation needed. 

TL:DR update of current position:

What works:

- can charge pack, can drive vacuum cleaner, including the USART comms to keep the cleaner working
- Probably a fair number of odd glitches though

Problems
- doesn't use the rest of the status LEDs to reflect cleaner problems eg Blocked/Filter statuses
- doesn't use the coulomb charge counter to monitor pack charge, just displays guestimates based on cell voltage.

The biggie:

Can't flash it without an Atmel-ICE programmer and the (free) Microchip studio.   This is because OpenOCD won't 'unlock and erase' the chip protection on the chip by itself.
If we could fix that (should be possible in theory) then it MIGHT be possible to use a Raspberry Pi or similar to flash the pack!
