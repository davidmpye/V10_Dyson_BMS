# V10_Dyson_BMS

A written-from-scratch unofficial firmware for Dyson V10 Battery Management Systems

Allows __YOU__ to take control of your pack - rebuild it, use it to power something else, and best of all, __you can install the new firmware with just a Raspberry Pi__ - no need for expensive programmers!

Have a read of the [Wiki](https://github.com/davidmpye/V10_Dyson_BMS/wiki) for more information!

TL:DR update of current position:

What works:

- can charge pack, can drive vacuum cleaner, including the USART comms to keep the cleaner working
- error reporting if faults occur
- uses the rest of the status LEDs to reflect cleaner problems eg Blocked/Filter statuses
- uses the coulomb charge counter to monitor pack charge, just displays guestimates based on cell voltage.

Problems
- No cell balancing (same as stock), I doubt this can be fixed trivially.

![v10-closeup](https://github.com/davidmpye/V10_Dyson_BMS/assets/2261985/9c3c997c-1c46-4f77-aa3a-e4a8f9b940f4)

## Are you looking for support for Dyson V11 batteries?

If so, you need [this](https://github.com/vladislav1983/V11_Dyson_BMS/) !
