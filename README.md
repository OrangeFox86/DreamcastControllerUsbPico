# DreamcastControllerUsbPico
 Dreamcast to USB Gamepad Converter for Raspberry Pi Pico

Goals for this project:
- Detect and interact with the following:
  - Controller
  - VMU (or just MU)
  - Jump pack
- Setup USB HID Gamepad that supports vibration
- Setup some other USB device for VMU/MU access
- Create whatever Linux/Windows drivers are necessary to communicate with all devices (probably will just use libusb)
- Create Linux/Windows software to upload/download data to/from memory unit
- Interface with emulator such as Redream for controller, jump pack, and VMU

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
  - Vibration peripheral is working (not yet implemented on USB side)
    - I'm looking into how to support force feedback on an HID gamepad (more universal but likely less supported) while also looking into supporting XInput as a secondary option
  - Stubs added for all other peripherals (some to be filled in later)
- Added transmission timeliner so that any peripheral addition is scalable without worrying much about how peripherals may interfere with each other

## Why the RP2040 is a Game Changer

To emulate a bespoke bus such as the Maple Bus on an MCU, one would usually either need to add extra hardware or bit bang the interface. This is not true with the RP2040 and its PIO. Think of it as several extra small processors on the side using a special machine language purpose-built for handling I/O. This means I can offload communication to the PIO and only check on them after an interrupt is activated or a timeout has elapsed. Check out [maple.pio](src/hal/maple.pio) to see the PIO code.

Luckily, the RP2040 comes with 2 PIO blocks each with 4 separate state machines. This means that the RP2040 can easily emulate 4 separate controller interfaces! This project uses one PIO block for writing and one for reading. This is necessary because each PIO block can only hold up to 32 instructions. The write state machine is completely stopped before starting the read state machine for the targeted bus. This wouldn't be necessary if read was done on separate pins from write as each PIO needs to take ownership of the pins. More simplistic wiring is desired if possible though. Switching state machines is fast enough that there shouldn't be a problem, especially since the "write" state machine intentionally doesn't bring the bus back to neutral before it notifies the application. Then the application side kills the "write" state machine and brings the bus to neutral just before switching to "read". The application-side write completion process should happen within a microsecond. A device on the Maple Bus usually starts responding after 50 microseconds from the point of the bus going neutral at the end of an end sequence. This ensures that a response is always captured.

# Build Instructions (for Linux and Windows)

If running under Windows, install [WSL](https://docs.microsoft.com/en-us/windows/wsl/install) and your desired flavor of Linux. I recommend using Ubuntu 20.04 as that is what I have used for development. Then the steps below may be run within your WSL instance.

1. Install git, cmake, and gcc-arm-none-eabi compiler by running the following commands
```bash
sudo apt update
sudo apt -y install git cmake gcc-arm-none-eabi
```
2. (optional) In order to run and debug tests, install standard gcc compilers and gdb by running the following
```bash
sudo apt -y install build-essential gdb
```
3. Clone this repo then cd into the created directory
```bash
git clone https://github.com/Tails86/DreamcastControllerUsbPico.git
cd DreamcastControllerUsbPico
```
4. Pull down the pico SDK (this is optional if you have PICO_SDK_PATH set in your environment which points to the SDK somewhere on your system)
```bash
git submodule update --recursive --init
```
5. (optional) Build and run tests - this runs core lib unit tests locally
```bash
./run_tests.sh
```
6. Execute the build script
```bash
./build.sh
```

After build completes, the binary should be located at `dist/main.uf2`.

This project may be opened in vscode. In vscode, the default shortcut `ctrl+shift+b` will build the project. The default shortcut `F5` will run tests with gdb for local debugging. Open the terminal tab after executing tests with debugging to see the results.

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

The Dreamcast uses 36 ohm resistors and small fuses in series between the 8 chip I/O and the 8 controller port I/O. I have decided to use lower value resistors because some resistance is built into the pico's outputs already. There is some weirdness with plugging in a VMU without a battery installed which gets worse with higher resistor values. I'm using littelfuse 1/16 A quick burning fuses, and none of them have blown during my tests. They're mostly optional, but I'd suggest using higher value resistors if they are omitted as a safety measure. Anything higher than 100-ohm starts to cause communication errors though due to capacitances on the I/O.

This is generally the setup I have been testing with:

<p align="center">
  <img src="images/schematic.png?raw=true" alt="Schematic"/>
</p>

## Generating Maple Bus Output

The maple_out PIO state machine handles Maple Bus output.

### Start Sequence

Every packet begins with a start sequence. Note that there are different start sequences which determine how the following bit sequence should be handled, but this project mainly focuses on the standard start sequence used to communicate with a standard Dreamcast controller and its sub-peripherals.

<p align="center">
  <img src="images/Maple_Bus_Start_Sequence.png?raw=true" alt="Maple Bus Start Sequence"/>
</p>

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

# Maple Bus Packet

## Word Format

Each word is 32 bits in length, transmitted in little-endian byte order. The most significant bit of each byte transmits first. This means that the most significant bit of the least significant byte of each word transmits first. Refer to the [Frame Word](#frame-word) section for an example of how a word is formed.

When ASCII text is transmitted, the most significant byte is the first character of the 4 character sequence in each word. This means that the byte order of each word needs to be flipped before parsing the payload as a character array. The size of the ASCII payload section is pre-determined based on the command. No NULL termination byte is supplied at the end of the string, and spaces are used to pad out remaining characters at the end of the string.

## Packet Data Format

A packet consists of the following data.

* **[Frame](#frame-word):** 1 32-Bit Word
* **[Payload](#payload):** 0 to 255 32-Bit Words
* **[CRC](#crc):** 1 Byte

### Frame Word

The following is how a frame word is broken down into its 4 parts.

| Byte 0 (LSB) | Byte 1 | Byte 2 | Byte 3 (MSB) |
| :---: | :---: | :---: | :---: |
| Number of Words<br>in Payload | Sender<br>[Address](#addressing) | Recipient<br>[Address](#addressing) | [Command](#commands) |

example:

<p align="center">
  <img src="images/Frame_Word.png?raw=true" alt="Frame Word"/>
</p>

#### Addressing

The following addresses are used for all components on the bus.

| Player Number | Host | Main Peripheral | Sub-Peripheral 1 | Sub-Peripheral 2 | Sub-Peripheral 3 | Sub-Peripheral 4 | Sub-Peripheral 5 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 1 | 0x00 | 0x20* | 0x01 | 0x02 | 0x04 | 0x08 | 0x10 |
| 2 | 0x40 | 0x60* | 0x41 | 0x42 | 0x44 | 0x48 | 0x50 |
| 3 | 0x80 | 0xA0* | 0x81 | 0x82 | 0x84 | 0x88 | 0x90 |
| 4 | 0xC0 | 0xE0* | 0xC1 | 0xC2 | 0xC4 | 0xC8 | 0xD0 |

*When the main peripheral sets its sender address, it also sets the bits corresponding to which sub-peripherals are attached. For example, if sub-peripherals 1 and 2 are attached to player 1's main peripheral, the main peripheral's sender address will be 0x23. This informs the host what else is attached.

The peripheral may respond with a source address as if it is player 1. As such, the host should ignore whatever the upper 2 bits that the device uses as its source address.

#### Commands

| Command Value | Description | Communication Direction | Number of Payload Words | Expected Response |
| :---: | :---: | :---: | :---: | :---: |
| 0x01 | Device Info Request* | Host->Device | 0 | 0x05 |
| 0x02 | Extended Device Info Request | Host->Device | 0 | 0x06 |
| 0x03 | Reset | Host->Device | 0 | 0x07 |
| 0x04 | Shutdown | Host->Device | 0 | 0x07 |
| 0x05 | Device Info | Device->Host | [28](#device-info-payload-structure-cmd-0x05) | - |
| 0x06 | Extended Device Info | Device->Host | [48](#extended-device-info-payload-structure-cmd-0x06) | - |
| 0x07 | Acknowledge | Device->Host | 0 | - |
| 0x08 | Data Transfer | Device->Host | [2..255](#data-transfer-payload-structure-cmd-0x08) | - |
| 0x09 | Get Condition | Host->Device | [1](#get-condition-payload-structure-cmd-0x09) | 0x08 |
| 0x0A | Get Memory Information | Host->Device | [2](#get-memory-information-payload-structure-cmd-0x0a) | 0x08 |
| 0x0B | Block Read | Host->Device | [2](#block-read-payload-structure-cmd-0x0b) | 0x08 |
| 0x0C | Block Write | Host->Device | [3..255](#block-write-payload-structure-cmd-0x0c) | 0x07 |
| 0x0E | Set Condition | Host->Device | [2..255](#set-condition-payload-structure-cmd-0x0e) | 0x07 |
| 0xFB | File Error | Device->Host | 0 | - |
| 0xFC | Request Resend | Device->Host | 0 | - |
| 0xFD | Unknown Command | Device->Host | 0 | - |
| 0xFE | Function Code Not Supported | Device->Host | 0 | - |

*Most peripheral devices won't respond to any other command until device info is requested for the device.

### Payload

The structure of a payload is structured based on the command used in the frame word.

#### Device Info Payload Structure (cmd 0x05)

| Word 0 | Words 1-3 | Word 4 | Words 5-11 | Words 12-26 | Word 27 |
| :---: | :---: | :---: | :---: | :---: | :---: |
| Supported [function codes](#function-codes) mask* | ??? | 2 least significant bytes: first two characters of description ASCII string** | The rest of the description ASCII string** | Producer information ASCII string** | ??? |

*The supported function codes mask in device info responses will contain the bitmask for 1 or more devices ex: a VMU will have a mask of 0x0000000E for Timer, Screen, and Storage.

**Refer to the [word format](#word-format) section about how to parse ASCII strings.

#### Extended Device Info Payload Structure (cmd 0x06)

| Word 0 | Words 1-3 | Word 4 | Words 5-11 | Words 12-26 | Word 27 | Words 28-47 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Supported [function codes](#function-codes) mask* | ??? | 2 least significant bytes: first two characters of description ASCII string** | The rest of the description ASCII string** | Producer information ASCII string** | ??? | Version information and/or capabilities ASCII string** |

*The supported function codes mask in device info responses will contain the bitmask for 1 or more devices ex: a VMU will have a mask of 0x0000000E for Timer, Screen, and Storage.

**Refer to the [word format](#word-format) section about how to parse ASCII strings.

#### Data Transfer Payload Structure (cmd 0x08)

| Word 0 | Words 1..255 |
| :---: | :---: |
| [Function code](#function-codes) | Data - device dependent structure |

#### Get Condition Payload Structure (cmd 0x09)

| Word 0 |
| :---: |
| [Function code](#function-codes) |

#### Get Memory Information Payload Structure (cmd 0x0A)

| Word 0 | Word 1 |
| :---: | :---: |
| [Function code](#function-codes) | [Location word](#location-word) |

#### Block Read Payload Structure (cmd 0x0B)

| Word 0 | Word 1 |
| :---: | :---: |
| [Function code](#function-codes) | [Location word](#location-word) |

#### Block Write Payload Structure (cmd 0x0C)

| Word 0 | Word 1 | Words 2..255 |
| :---: | :---: | :---: |
| [Function code](#function-codes) | [Location word](#location-word) | Data - device dependent structure |

#### Set Condition Payload Structure (cmd 0x0E)

| Word 0 | Words 1..255 |
| :---: | :---: |
| [Function code](#function-codes) | Condition - device dependent structure |

#### Common Payload Word Types

##### Function Codes

The below are function codes which are used to address functionality in some payloads.

| Code / Mask | Description |
| :---: | :---: |
| 0x00000001 | Controller |
| 0x00000002 | Storage |
| 0x00000004 | Screen |
| 0x00000008 | Timer |
| 0x00000010 | Audio Input |
| 0x00000020 | AR Gun |
| 0x00000040 | Keyboard |
| 0x00000080 | Gun |
| 0x00000100 | Vibration |
| 0x00000200 | Mouse |

##### Location Word

Below defines a location word which is used to address blocks of memory in some peripherals.

| Byte 0 (LSB) | Byte 1 | Byte 2 | Byte 3 (MSB) |
| :---: | :---: | :---: | :---: |
| Block | 0x00 | Phase | Partition |

* **Block**: Memory block number index
* **Phase**: Sequence number
* **Partition**: Partition number (normally 0)

### CRC

CRC byte transmits last, just before the end sequence is transmitted. It is the value after starting with 0 and applying XOR to each other byte in the packet.

# External Resources

**Maple Bus Resources**

http://mc.pp.se/dc/maplebus.html and http://mc.pp.se/dc/controller.html

https://tech-en.netlify.app/articles/en540236/index.html

https://www.raphnet.net/programmation/dreamcast_usb/index_en.php
