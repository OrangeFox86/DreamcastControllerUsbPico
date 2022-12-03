# DreamcastControllerUsbPico

Dreamcast Controller to USB Gamepad Converter using Raspberry Pi Pico

Goals for this project:
- Detect and interact with the following:
  - Controller
  - VMU
  - Jump pack
- Setup USB HID Gamepad that supports vibration
- Setup some other USB device for VMU access
- Create whatever Linux/Windows drivers are necessary to communicate with all devices (probably will just use libusb)
- Interface with emulator such as Redream for controller, jump pack, and VMU

Refer to the [releases](https://github.com/OrangeFox86/DreamcastControllerUsbPico/releases) page for current progress. Refer to the [issues](https://github.com/OrangeFox86/DreamcastControllerUsbPico/issues) tab for things left to be implemented and known bugs.

## Why the RP2040 is a Game Changer (and what makes this project different from others)

To emulate a bespoke bus such as the Maple Bus on an MCU, one would usually either need to add extra hardware or bit bang the interface. This is not true with the RP2040 and its PIO. Think of it as several extra small processors on the side using a special machine language purpose-built for handling I/O. This means communication can be offloaded to the PIO and only check on them after an interrupt is activated or a timeout has elapsed. Check out [maple_in.pio](src/hal/MapleBus/maple_in.pio) and [maple_out.pio](src/hal/MapleBus/maple_out.pio) to see the PIO code.

Luckily, the RP2040 comes with 2 PIO blocks each with 4 separate state machines. This means that the RP2040 can easily emulate 4 separate controller interfaces, each at full speed!

---

# Quick Installation Guide

## Connecting the Hardware

The Dreamcast uses 36-ohm resistors and small fuses in series between the 8 chip I/O and the 8 controller port I/O. This is only done as a failsafe in case of hardware or software malfunction. In my implementation, I have decided to use lower value, 10-ohm resistors because some resistance is built into the Pico's outputs already. There is some weirdness with plugging in a VMU without a battery installed which gets worse with higher resistor values. I'm also using littelfuse 1/16-amp quick burning fuses, and none of them have blown during my tests. They're mostly optional, but I'd suggest using higher value resistors if they are omitted as a safety measure. Anything higher than 100-ohm starts to cause communication errors though due to capacitances on the I/O.

This is generally the setup I have been testing with:

<p align="center">
  <img src="images/schematic.png?raw=true" alt="Schematic"/>
</p>

For reference, the following is the pinout for the Dreamcast controller port. Take note that many other sources found online refer to one of the ground pins as a connection sense, but the Dreamcast controller port module has both of these ground pins hard wired together. As such, this project doesn't rely on any such hardware sense line. Instead, the detection of a connected device is performed by polling the bus until a response is received, just as a real Dreamcast would.

<p align="center">
  <img src="images/Dreamcast_Port.png?raw=true" alt="Dreamcast Port"/>
</p>

## Build Instructions (for Linux and Windows)

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

After build completes, the binary should be located at `dist/main.uf2`. Pre-built release binaries may be found [here](https://github.com/OrangeFox86/DreamcastControllerUsbPico/releases).

This project may be opened in vscode. In vscode, the default shortcut `ctrl+shift+b` will build the project. The default shortcut `F5` will run tests with gdb for local debugging. Open the terminal tab after executing tests with debugging to see the results.

## Loading the UF2 Binary

Hold the BOOTSEL button on the Pico while plugging the USB connection into your PC. A drive with a FAT partition labeled RPI-RP2 should pop up on your system. Open this drive, and then copy the main.uf2 file here. The Pico should then automatically load the binary into flash and run it. For more information, refer to the official [Raspberry Pi Pico documentation](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html#documentation).

---

# Maple Bus Implementation

Maple Bus is the name of the bus used for the controller interface on the Dreamcast. This project simulates a host communicating on a maple bus. All of the code pertaining to the Maple Bus implementation is located under [src/hal/MapleBus](src/hal/MapleBus).

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

## Interfacing with the PIO State Machines

The [MapleBus class](src/hal/MapleBus/MapleBus.hpp) operates as the interface between the microcontroller's code and the PIO state machines, [maple_in.pio](src/hal/MapleBus/maple_in.pio) and [maple_out.pio](src/hal/MapleBus/maple_out.pio).

Using 2 separate PIO blocks for reading and writing is necessary because each PIO block can only hold up to 32 instructions, and this interface is too complex to fit both read and write into a single block. Therefore, the write state machine is completely stopped before starting the read state machine for the targeted bus. Switching state machines is fast enough that there shouldn't be a problem. Testing showed the handoff always occurs within 1 microsecond after bringing the bus back to neutral. A device on the Maple Bus starts responding some time after 50 microseconds from the point of the bus going neutral after an end sequence. This ensures that a response is always captured.

The following lays out the phases of the state machine handled within the MapleBus class.

<p align="center">
  <img src="images/MapleBus_Class_State_Machine.png?raw=true" alt="MapleBus Class State Machine"/>
</p>

### PIO Data Handoff

When the write method is called, data is loaded into the Direct Memory Access (DMA) channel designated for use with the maple_out state machine in the MapleBus instance. The DMA will automatically load data onto the TX FIFO of the output PIO state machine so it won't stall waiting for more data.

The first 32-bit word loaded onto the output DMA is how many transmission bits will follow. In order for the state machine to process things properly, `(x - 8) % 32 == 0 && x >= 40` must be true where x is the value of that first 32-bit word i.e. every word is 32 bits long and at least a frame word (32 bits) plus a CRC byte (8 bits) are in the packet. This value needs to be loaded with byte order flipped because byte swap is enabled in the DMA so that all other words are written in the correct byte order. The rest of the data loaded into DMA is the entirety of a single packet as a uint32 array. The last uint32 value holds the 8-bit CRC.

A blocking IRQ is triggered once the maple_out state machine completes the transfer. This then allows MapleBus to stop the maple_out state machine and start the maple_in state machine.

A Direct Memory Access (DMA) channel is setup to automatically pop items off of the RX FIFO of the maple_in state machine so that the maple_in state machine doesn't stall while reading. Once the IRQ is triggered by the maple_in state machine, MapleBus stops the state machine and reads from data in the DMA.

## Generating Maple Bus Output

The [maple_out PIO state machine](src/hal/MapleBus/maple_out.pio) handles Maple Bus output.

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

The [maple_in PIO state machine](src/hal/MapleBus/maple_in.pio) handles Maple Bus input. Some concessions had to be made in order to handle all input operations within the 32 instruction set limit of the input PIO block. The following are the most notable limitations.
- Only a standard data packet may be sampled
    - The Maple Bus protocol has different types of packets depending on how many times B pulses in the start sequence, but those packets are ignored in this implementation
- The full end sequence is not sampled
    - The packet length in the frame word plus the CRC are relied upon during post-processing in order to verify that the received packet is valid

### Sampling the Start Sequence

The input PIO state machine will wait until **A** transitions LOW and then count how many times **B** toggles LOW then HIGH while making sure **A** doesn't transition HIGH until after **B** transitions HIGH. If the toggle count isn't 4, then the state machine keeps waiting. Otherwise, the state machine signals the application with a non-blocking IRQ and continues to the next phase where data bits are sampled.

### Sampling Data Bits

For each bit, the state machine first waits for the designated clock to be HIGH before proceeding. Then once this line transitions to LOW, the state of the designated data line is sampled. State transitions of the designated data line are ignored except for the case when sensing the end sequence is required as described in the next section.

### Sampling the End Sequence

Whenever **A** is designated as the clock, the input PIO state machine will detect when **B** toggles HIGH then LOW while **A** remains HIGH. It is assumed that this is the beginning of the end sequence since this is not a normal behavior during data transmission. The state machine will then block on an IRQ so that the application can handle the received data.

---

# Maple Bus Packet

This section is included for reference. It contains information about packet structure which was used to build up packets within components of [coreLib](src/coreLib/).

## Word Format

Each word is 32 bits in length, transmitted in little-endian byte order. The most significant bit of each byte transmits first. This means that the most significant bit of the least significant byte of each word transmits first. Refer to the [Frame Word](#frame-word) section for an example of how a word is formed. All tables in this document list bytes in transmission order with the least significant bit (LSB) as the first byte.

When ASCII text or a byte stream is transmitted, the most significant byte is the first character of the 4 character sequence in each word. This means that the byte order of each word needs to be flipped before parsing the payload as a character or byte array. The size of an ASCII payload section is pre-determined based on the command. No NULL termination byte is supplied at the end of the string, and spaces are used to pad out remaining characters at the end of the string.

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

*When the main peripheral responds with its sender address, it also sets the bits corresponding to which sub-peripherals are attached. For example, if sub-peripherals 1 and 2 are attached to player 1's main peripheral, the main peripheral will set its sender address to 0x23. This informs the host what else is attached. The host should still set the recipient address to 0x20 when sending data to this peripheral though.

In testing, there have been cases where a peripheral will respond with a source address as if it is player 1. As such, the host should ignore whatever the upper 2 bits that the device uses as its source address.

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

---

# Peripheral Implementation

## Controller

The "get condition" command (0x09) with controller function code is used to poll controller state. A "data transfer" command (0x08) is returned with controller function code word plus 2 words of data.

### First Data Transfer Word

All button bits are 0 when pressed and 1 when released.

| Byte 0 (LSB) | Byte 1 | Byte 2 | Byte 3 (MSB) |
| :---: | :---: | :---: | :---: |
| Left analog trigger 0 to 255<br>0: fully released<br>255: fully pressed | Right analog trigger 0 to 255<br>0: fully released<br>255: fully pressed | Button bits<br>0x01: Z<br>0x02: Y<br>0x04: X | Button bits<br>0x01: C<br>0x02: B<br>0x04: A<br>0x08: Start<br>0x10: Up<br>0x20: Down<br>0x40: Left<br>0x80: Right |

### Second Data Transfer Word

| Byte 0 (LSB) | Byte 1 | Byte 2 | Byte 3 (MSB) |
| :---: | :---: | :---: | :---: |
| Right analog stick up/down<br>Always 128<br>(no controller uses this) | Right analog stick left/right<br>Always 128<br>(no controller uses this) | Left analog up/down<br>0: fully up<br>128: center<br>255: fully down | Left analog left/right<br>0: fully left<br>128: center<br>255: fully right

## Screen

The "block write" command (0x0C) with screen function code and 48 data words is used to write monochrome images to the screen. A screen is 48 bits wide and 32 bits tall. For each bit in the 48 data words, a value of 1 means the pixel is on (black) and 0 means the pixel is off (white). Data is written from left to right and top to bottom. The most significant bit of the first word sets the pixel on the top, left of the screen. The two most significant bytes write to the 33rd through 48th bit of the first row. The next two bytes write to the 1st through 16th bits of the second row. This is repeated for the right of the 48 words like pictured below.

<p align="center">
  <img src="images/Dreamcast_Screen_Words.png?raw=true" alt="Screen Words"/>
</p>

## Storage

The "block read" and "block write" commands (0x0B and 0x0C) with storage function code are used to read and write the 256 blocks of memory in the storage peripheral. There are 256 pages of memory that make up the entire storage space. Each page consists of 512 bytes. That makes a total of 128 KB of memory.

## Vibration

The "set condition" command (0x0E) with vibration function code and 1 condition word is used to activate vibration.

Take note that the following understanding was made from a lot of trial and error, so it is very possible this isn't completely correct. This has been thoroughly tested to work well enough on the following devices though.
- Sega OEM Puru Puru Pack
- Performance TremorPak

### Condition Word

The following describes the layout of the condition word in the vibration "set condition" command.

| Byte 0 (LSB) | Byte 1 | Byte 2 | Byte 3 (MSB) |
| :---: | :---: | :---: | :---: |
| [Vibration Cycles](#vibration-cycles-byte-0) | [Pulsation Frequency](#pulsation-frequency-byte-1) | [Inclination Direction and Power](#inclination-direction-and-power-byte-2) | [Vibration Mode](#vibration-mode-byte-3) |

#### Vibration Cycles (Byte 0)

This value represents how many pulsation cycles to execute per inclination intensity. The number of cycles to execute is 1 more than the value specified. With inclination set, this value can be set to a maximum of 255. This value must be 0 when no inclination is set, so only a single cycle will execute in that case.*

*The Performance TremorPak allows this value to be set to up to 255 when no inclination is set, but the OEM pack enforces this limitation.

#### Pulsation Frequency (Byte 1)

This value sets the frequency at which the motor pulsates. The pulsation is smooth - feels like a sine wave pulsation. At values near the maximum, the pulsation is not very noticeable at all.

The valid range for this value is between 7 and 59. The extended device info from a vibration pack was very helpful in determining the appropriate frequency based on this value:

```
Puru Puru Pack
Produced By or Under License From SEGA ENTERPRISES,LTD.
Version 1.000,1998/11/10,315-6211-AH   ,Vibration Motor:1 , Fm:4 - 30Hz ,Pow:7
```

Specifically, the text `Fm:4 - 30Hz`. This correlates to `(value + 1) / 2` and matches what was observed in testing.

#### Inclination Direction and Power (Byte 2)

- For the following, the value of X may be in the range [1,7] where 1 is low power and 7 is high power
    - `0xX0`: single stable vibration (i.e. no inclination) at power X
    - `0xX8`: positive inclination (ramp up) from power X up to max
    - `0x8X`: negative inclination (ramp down) from power X down to min
- A values of `0x00`, `0x08`, or `0x80` immediately stops the currently executing vibration sequence

There is a very noticeable change from one vibration power to the next when inclination is used and a long cycle period is selected.

#### Vibration Mode (Byte 3)

This byte must be set to the following.

| Bit 7 <br> (MSb) | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 <br> (LSb) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 0 | 0 | 0 | 1 | 0 | 0 | 0 | X |

Bit 0 may be set to 1 to augment duration, but the meaning is not completely understood. As such, that bit is always set to 0 for this implementation.

---

# Appendix A: Abbreviations and Definitions

- `0x` Prefix: The following value is hex format
- Byte: Data consisting of 8 consecutive bits
- DMA: Direct Memory Access
- LSB: Least Significant Byte
- LSb: Least Significant bit
- MSB: Most Significant Byte
- MSb: Most Significant bit
- Nibble: Data consisting of 4 consecutive bits
- PIO: Programmable Input/Output
- SDCK: Serial Data and Clock I/O
- Word: Data consisting of 32 consecutive bits

---

# External Resources

**Maple Bus Resources**

http://mc.pp.se/dc/maplebus.html and http://mc.pp.se/dc/controller.html

https://tech-en.netlify.app/articles/en540236/index.html

https://www.raphnet.net/programmation/dreamcast_usb/index_en.php
