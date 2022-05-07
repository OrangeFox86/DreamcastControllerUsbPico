# DreamcastControllerUsbPico
 Dreamcast to USB Gamepad Converter for Raspberry Pi Pico

This is a work in progress.
- There is no USB interface yet
- Read and Write communication with maple bus is complete!

## Why the RP2040 is a Game Changer

To emulate a bespoke bus such as the Maple Bus on an MCU, one would usually either need to add extra hardware or bit bang the interface. This is not true with the RP2040 and its PIO. Think of it as several extra small processors on the side using a special machine language purpose-built for handling I/O. This means I can offload communication to the PIO and only check on them after an interrupt is activated or a timeout has elapsed. Check out [maple.pio](maple.pio) to see the PIO code.

Luckily, the RP2040 comes with 2 PIO blocks each with 4 separate state machines. This means that the RP2040 can easily emulate 4 separate controller interfaces! This project uses one PIO block for writing and one for reading. This is necessary because each PIO block can only hold up to 32 instructions. The write state machine is compeltely stopped before starting the read state machine for the targeted bus. This wouldn't be necessary if read was done on separate pins from write as each PIO needs to take ownership of the pins. More simplistic wiring is desired if possible though. Switching state machines is fast enough that there shouldn't be a problem, especially since the "write" state machine intentially doesn't bring the bus back to neutral before it notifies the application. Then the application side kills the "write" state machine and brings the bus to neutral just before switching to "read". The application-side write completion process should happen within a microsecond. A device on the Maple Bus usually starts responding after 50 microseconds from the point of the bus going neutral at the end of an end sequence. This ensures that a response is always captured.

## Generating Maple Bus Output

### Start and End Sequences

Every packet begins with a start sequence and is completed with an end sequence. Note that there are different start sequences which determine how the following bit sequence should be handled, but this project mainly focuses on the standard start sequence used to communicate with a standard Dreamcast controller and VMU.

![Maple_Bus_Start_and_End_Sequences](images/Maple_Bus_Start_and_End_Sequences.png)

#### Side Note on Start Sequence

Some sources claim that B transitioning LOW is part of the start sequence. However, the patent for Maple Bus shows that the start sequence ends when both A and B are HIGH. I will need to verify this by forcing some actual hardware to transmit 128 words of data which would make the first bit HIGH. The question is then: does the B line transition LOW then back HIGH before getting clocked or will it remain HIGH? According to the patent, I assume B should remain HIGH.

### Generating Data Bits

For each bit, one line of the maple bus acts as a clock while the other is the data to be sampled. The two lines trade their function for each bit. The first bit has A acting as clock and B acting as data. The next bit has B acting as clock and A acting as data. The one after that has A acting as clock again. The pattern repeats until all data is transmitted. A data bit is clocked when the designated clock line transitions from HIGH to LOW.

Each state transition can be broken down into 3 phases:
- Phase 1 - Clock Conditioning: Bring clock HIGH and keep data at the state it was previously
- Phase 2 - Data Conditioning: Transition the data bit to the target value
- Phase 3 - Clocking: Bring clock LOW in order to have the data bit sampled

![Maple_Bus_Clocking_Phases](images/Maple_Bus_Clocking_Phases.png?raw=true)

There are a total of 6 types of state transitions, depending on what the previous phase was. A depiction of state transitions can be seen in the image below.

![Maple_Bus_State_Truth_Table](images/Maple_Bus_State_Truth_Table.png?raw=true)

This allows for each line, A & B to transition in a staggard pattern. On the Dreamcast, each "phase" lasts about 160 nanoseconds which means each bit can be transmitted in about 480 nanoseconds. Because of the staggard pattern, the time between one edge and the next on each line is the sum of the time of 2 phases which is about 320 nanoseconds on the Dreamcast.

For reference, Dreamcast controllers usually transmit a little slower with each phase lasting about 250 nanoseconds with about 110 microsecond delays between each 3 word chunk. Timing is not so critical since each host/device on the bus controls its own clocking sequence. Still, it was a goal of this project to mimic the Dreamcast timing sequence as close as possible.

### Packet Data Format

(TODO)
- Each word is 32 bits in length transmitted as little endian with most significant bit transmitted first (i.e. the most significant bit of the least significant byte transmits first)
- Frame word consists of number of following words in packet (LSB) (8 bits), sender address (8 bits), recipient address (8 bits), and command (MSB) (8 bits)
- CRC byte transmits last and is the value after taking 0 and applying XOR to each byte in the packet
