# DreamcastControllerUsbPico
 Dreamcast to USB Gamepad Converter for Raspberry Pi Pico

Goals for this project:
- Detect and interact with the following:
  - Controller
  - VMU (or just MU)
  - Rumble Pack
- Setup USB HID Gamepad that supports rumble
- Setup some other USB device for VMU/MU access
- Create whatever Linux/Windows drivers are necessary to communicate with all devices (probably will just use libusb)
- Create Linux/Windows software to upload/download data to/from memory unit
- Interface with emulator such as Redream for controller, rumble pack, and VMU

*Please reach out to me if you want to help out with development or have any tips. The PC side of things intimidates me slightly. I think modifying the open source version of Redream is my only immediate option to actually get an emulator interface to work with this (to display VMU screens and possibly access VMU data). My voice gets lost in the Redream Discord server, so I don't think I'll get attention from that community until I can prove something works.*

This is a work in progress. Current progress:
- Read and Write communication with maple bus works!
- The USB interface works for 4 gamepad devices only (no vibration, VMU, or any other peripheral yet)
- Dreamcast nodes dynamically handle different peripherals
  - Controller detection works
    - I have successfully played Steam games on my Windows PC using a Dreamcast controller with this
    - Standard controller works
    - C and Z buttons on arcade stick also work
  - Screen interface is working (the "V" in "VMU")
    - Currently just a default screen is sent on connection

## Why the RP2040 is a Game Changer

To emulate a bespoke bus such as the Maple Bus on an MCU, one would usually either need to add extra hardware or bit bang the interface. This is not true with the RP2040 and its PIO. Think of it as several extra small processors on the side using a special machine language purpose-built for handling I/O. This means I can offload communication to the PIO and only check on them after an interrupt is activated or a timeout has elapsed. Check out [maple.pio](src/hal/maple.pio) to see the PIO code.

Luckily, the RP2040 comes with 2 PIO blocks each with 4 separate state machines. This means that the RP2040 can easily emulate 4 separate controller interfaces! This project uses one PIO block for writing and one for reading. This is necessary because each PIO block can only hold up to 32 instructions. The write state machine is completely stopped before starting the read state machine for the targeted bus. This wouldn't be necessary if read was done on separate pins from write as each PIO needs to take ownership of the pins. More simplistic wiring is desired if possible though. Switching state machines is fast enough that there shouldn't be a problem, especially since the "write" state machine intentionally doesn't bring the bus back to neutral before it notifies the application. Then the application side kills the "write" state machine and brings the bus to neutral just before switching to "read". The application-side write completion process should happen within a microsecond. A device on the Maple Bus usually starts responding after 50 microseconds from the point of the bus going neutral at the end of an end sequence. This ensures that a response is always captured.

# Build Instructions (for Linux and Windows)

If running under Windows, install [WSL](https://docs.microsoft.com/en-us/windows/wsl/install) and your desired flavor of Linux. I recommend using Ubuntu 20.04 as that is what I have used for development. Then the steps below may be run within your WSL instance.

1. Install git, cmake, standard gcc compilers, and gcc-arm-none-eabi compiler by running the following commands
```bash
sudo apt update
sudo apt -y install git cmake build-essential gcc-arm-none-eabi
```
2. Clone this repo into your WSL instance
```bash
git clone https://github.com/Tails86/DreamcastControllerUsbPico.git
```
3. Go into the project's directory and pull down the pico SDK (this is optional if you have PICO_SDK_PATH set in your environment which points to the SDK somewhere on your system)
```bash
cd DreamcastControllerUsbPico
git submodule update --recursive --init
```
4. Execute the build script
```bash
./build.sh
```

After build completes, the binary should be located at `dist/main.uf2`

# Maple Bus Implementation

**Disclaimer:** I'm still working through this interface, so information here is not guaranteed to be 100% accurate.

Maple Bus is the name of the bus used for the controller interface on the Dreamcast.

## Hardware Overview

A Maple Bus consists of 2 signal/clock lines that are labeled SDCKA and SDCKB. Hardware on the Maple Bus consists of one host, zero or one main peripheral, and zero to five sub-peripherals. The only difference between a main peripheral and a sub-peripheral is that a main peripheral communicates to the host what sub-peripherals are attached during normal communication. The main peripheral is something like a Dreamcast controller, and the sub-peripherals are things like a VMU, rumble pack, and microphone. The host and all connected peripheral devices communicate on the same 2-line Maple Bus.

<p align="center">
  <img src="images/Maple_Bus_Electronics_Block_Diagram.png?raw=true" alt="Maple Bus Electronics Block Diagram"/>
</p>

- Both lines on the Bus are pulled HIGH through weak pullup resistors
- Only one connected component on the bus may communicate at a time
- During communication, a device should not drive both lines HIGH for very long to prevent a downstream device from thinking the bus is free
- Before a component starts communicating, it must verify the bus is neutral for a sufficient amount of time
- A peripheral device will only communicate 1 packet of data in response to a request from the host

<p align="center">
  <img src="images/Maple_Bus_Hardware_Communication.png?raw=true" alt="Maple Bus Hardware Communication"/>
</p>

### Connecting the Hardware

The Dreamcast uses a 36 ohm resistor and a small fuse in series between the chip I/O and the controller ports. In my tests, I have hard wired the I/O of the pico to the controller port without running into issues (yet). However, I plan on using a 33 ohm resistor and 1/16 amp littelfuse once I actually build things out. These components would be there just in case a faulty device was plugged in. On a fault, the 33 ohm resistor will limit the current draw to 100 mA at 3.3 V for each I/O. This is still twice the rated max I/O current of the RP2040. That's where the fuse will come in - it should blow within a few milliseconds (on average) after a fault occurs. I don't know how much abuse the RP2040 can take, but I assume it should be able to survive that. You'd then just be left with a dead port instead of a dead chip.

I played around with higher resistance on these lines, but any more than around 100 ohms limits the charge/discharge speed and causes communication errors. I feel like low value resistors and quick blowing fuses provide the best tradeoff while still keeping cost and complexity low. It also matches Sega's design.

## Generating Maple Bus Output

The maple_out PIO state machine handles Maple Bus output.

### Start Sequence

Every packet begins with a start sequence. Note that there are different start sequences which determine how the following bit sequence should be handled, but this project mainly focuses on the standard start sequence used to communicate with a standard Dreamcast controller and its sub-peripherals.

<p align="center">
  <img src="images/Maple_Bus_Start_Sequence.png?raw=true" alt="Maple Bus Start Sequence"/>
</p>

#### Side Note on Start Sequence

Some sources claim that B transitioning LOW is part of the start sequence. However, the patent for Maple Bus shows that the start sequence ends when both A and B are HIGH. I will need to verify this by forcing some actual hardware to transmit 128 words of data which would make the first bit HIGH. The question is then: does the B line transition LOW then back HIGH before getting clocked or will it remain HIGH? According to the patent, I assume B should remain HIGH.

### End Sequence

Every packet is completed with an end sequence to commit the data to the target component.

<p align="center">
  <img src="images/Maple_Bus_End_Sequence.png?raw=true" alt="Maple Bus End Sequence"/>
</p>

### Generating Data Bits

For each bit, one line of the maple bus acts as a clock while the other is the data to be sampled. A data bit is clocked when the designated clock line transitions from HIGH to LOW. The two lines trade their function after each bit. Line **A** acts as clock and **B** acts as data for the first bit. Line **B** acts as clock and **A** acts as data for the next bit. Line **A** acts as clock again for the bit after that. The pattern repeats until all data is transmitted.

Each state transition can be broken down into 3 phases:
- Phase 1 - Clock Conditioning: Bring clock HIGH and keep data at the state it was previously
- Phase 2 - Data Conditioning: Transition the data bit to the target value
- Phase 3 - Clocking: Bring clock LOW in order to have the data bit sampled

<p align="center">
  <img src="images/Maple_Bus_Clocking_Phases.png?raw=true" alt="Maple Bus Clocking Phases"/>
</p>

There are a total of 6 types of state transitions, depending on what the previous phase was. A depiction of state transitions can be seen in the image below.

<p align="center">
  <img src="images/Maple_Bus_State_Truth_Table.png?raw=true" alt="Maple Bus State Truth Table"/>
</p>

Notice that each line, A & B transitions states in a staggard pattern. On the Dreamcast, each "phase" lasts about 160 nanoseconds which means each bit can be transmitted in about 480 nanoseconds. Because of the staggard pattern, the minimum time between one edge and the next on each line is the sum of the time of 2 phases which is about 320 nanoseconds on the Dreamcast.

For reference, Dreamcast controllers usually transmit a little slower with each phase lasting about 250 nanoseconds with about 110 microsecond delays between each 3 word chunk after the first frame word.

## Sampling Maple Bus Input

The maple_in PIO state machine handles Maple Bus input. Some concessions had to be made in order to handle all input operations within the 32 instruction set limit of the input PIO block.

### Sampling the Start Sequence

The input PIO state machine will wait until **A** transitions LOW and then count how many times **B** toggles LOW then HIGH while making sure **A** doesn't transition HIGH until after **B** transitions HIGH. If the toggle count isn't 4, then the state machine keeps waiting. Otherwise, the state machine triggers a non-blocking IRQ to signal to the application that the state machine is now reading data.

### Sampling Data Bits

For each bit, the state machine waits for the designated clock to be HIGH then transition LOW before sampling the state of the designated data line. State transitions of the designated data line is ignored except for the case of sensing the end sequence as described in the next section.

### Sampling the End Sequence

Whenever **A** is designated as the clock, the input PIO state machine will detect when **B** toggles HIGH then LOW while **A** remains HIGH. It is assumed that this is the beginning of the end sequence. The state machine will then block on an IRQ so that the application can handle the received data. The application must then stop the input state machine.

## Packet Data Format

A packet consists of the following data.

| Frame | Payload | CRC |
| :---: | :---: | :---: |
| 1 32-Bit Word  | 0 to 255 32-Bit Words | 1 Byte |

### Word Format

Each word is 32 bits in length, transmitted in little-endian order. The most significant bit of each byte transmits first. This means that the most significant bit of the least significant byte of each word transmits first.

### Frame Word

The following is how a frame word is broken down into its 4 parts.

| Byte 0 (LSB) | Byte 1 | Byte 2 | Byte 3 (MSB) |
| :---: | :---: | :---: | :---: |
| Number of Words<br>in Payload | Sender<br>Address | Recipient<br>Address | Command |

example:

<p align="center">
  <img src="images/Frame_Word.png?raw=true" alt="Frame Word"/>
</p>

#### Addressing

The following addresses are used for all components on the bus.

| Host | Main Peripheral | Sub-Peripheral 1 | Sub-Peripheral 2 | Sub-Peripheral 3 | Sub-Peripheral 4 | Sub-Peripheral 5 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 0x00 | 0x20* | 0x01 | 0x02 | 0x04 | 0x08 | 0x10 |

*When the main peripheral sets its sender address, it also sets the bits corresponding to which sub-peripherals are attached. For example, if sub-peripherals 1 and 2 are attached, the main peripheral's sender address will be 0x23. This informs the host what else is attached.

#### Commands

| Command Value | Description |
| :---: | :---: |
| 0x01* | Device Info Request |
| 0x02 | Extended Device Info Request |
| 0x03 | Reset |
| 0x04 | Shutdown |
| 0x05 | Device Info |
| 0x06 | Extended Device Info |
| 0x07 | Acknowledge |
| 0x08 | Data Transfer |
| 0x09 | Get Condition |
| 0x0A | Get Memory Information |
| 0x0B | Block Read |
| 0x0C | Block Write |
| 0x0E | Set Condition |
| 0xFB | File Error |
| 0xFC | Request Resend |
| 0xFD | Unknown Command |
| 0xFE | Function Code Not Supported |

*Most peripheral devices won't respond to any other command until device info is requested for the device.

**TODO:** Add more info here

### CRC

CRC byte transmits last, just before the end sequence is transmitted. It is the value after starting with 0 and applying XOR to each other byte in the packet.
