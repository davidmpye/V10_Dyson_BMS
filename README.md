# V10_Dyson_BMS

A written-from-scratch unofficial firmware for Dyson V10 Battery Management Systems

Have a read of the [Wiki](https://github.com/davidmpye/V10_Dyson_BMS/wiki) for the most up to date information about the state of the project.

Lots more documentation needed. 

TL:DR update of current position:

What works:

- can charge pack, can drive vacuum cleaner, including the USART comms to keep the cleaner working
- Probably a fair number of odd glitches though

Problems
- doesn't use the rest of the status LEDs to reflect cleaner problems eg Blocked/Filter statuses
- doesn't use the coulomb charge counter to monitor pack charge, just displays guestimates based on cell voltage.
- doesn't check temperatures when charging the pack
- No cell balancing (same as stock), I doubt this can be fixed.
- Not very helpful about error codes at the moment!


![v10small](https://github.com/davidmpye/V10_Dyson_BMS/assets/2261985/1e88cf50-33de-437f-a9fb-07bd52d1e4b9)


