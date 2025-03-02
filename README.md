# 6502 Emulator

This project is a simple MOS 6502 microcontroller emulator with the following specifications:

- 8-bit CPU
- 64 Kb memory
- little endian layout
- adjustable clock speed

The emulator has all necessary registers, control flags, and memory operation capabilites.
It reads from a string of opcode instructions and executes these accordingly, manipulating registers, flags, and memory in realtime.

![emulator](https://github.com/connergrinstead/6502-emulator/blob/main/screenshot1.png)

# 6502 Assembler

Paired with the emulator comes a very simple assembler written in python.

This assembler takes classic assembly instructions and converts them to match the 6502's instruction set.
It includes all 13 addressing modes for all 151 documented opcodes.

![emulator](https://github.com/connergrinstead/6502-emulator/blob/main/screenshot2.png)
