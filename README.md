# DreamcastControllerUsbPico
 Dreamcast to USB Gamepad Converter for Raspberry Pi Pico

This is a work in progress.
- There is no USB interface yet
- Read and Write communication with maple bus is complete!

## Why the RP2040 is a Game Changer

To emulate a bespoke bus such as the Maple Bus on an MCU, one would usually either need to add extra hardware or bit bang the interface. This is not true with the RP2040 and its PIO. Think of it as several extra small processors on the side using a special machine language purpose-built for handling I/O. This means I can offload communication to the PIO and only check on them after an interrupt is activated or a timeout has elapsed. Check out maple.pio to see the PIO code.

Luckily, the RP2040 comes with 2 PIO blocks each with 4 separate state machines. This means that the RP2040 can easily emulate 4 separate controller interfaces! This project uses one PIO block for writing and one for reading. This is necessary because each PIO block can only hold up to 32 instructions. The write state machine is compeltely stopped before starting the read state machine. This wouldn't be necessary if read was done on separate pins from write as each PIO needs to take ownership of the pins. Switching like this is just fast enough that there shouldn't be a problem though. More simplistic wiring is desired if possible.
