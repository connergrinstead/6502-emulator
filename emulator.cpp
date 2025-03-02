#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <bitset>
#include <vector>
#include <unistd.h>

using namespace std;

/*
    |   |   |   |   |   |   |   |   |   |   |
-------------------------------------------------
|                                               |
|                 6502 Emulator                 |
|                                               |
-------------------------------------------------
    |   |   |   |   |   |   |   |   |   |   |

// 6502 Emulator
// 8 bit CPU
// 64Kb of memory - little endian
// Clock Speed of abot 1MHz to 3MHz

*/

using Byte = unsigned char;     // 8 bits
using Word = unsigned short;    // 16 bits
using u32 = unsigned int;

class Configuration {
    public:
        float clockSpeed = 1000;            // Clock Speed measure in Hz
        float simFactor = 0.002;            // Simulation Factor -> Sim Clock Speed = clockSpeed / simFactor
        int maxCycles = 30;
        bool verbose = false;
};

Configuration config;

struct Memory {
    static constexpr u32 MEM_SIZE = 1024 * 64;
    Byte Data[MEM_SIZE];

    Byte& operator[]( u32 address ) {
        return Data[address];
    }

    void Init( Byte e ) {
        for ( u32 i = 0; i < MEM_SIZE; i++ ) {
            Data[i] = e;
        }
        std::cout << "\n\033[1;31m-- Memory Init --\033[0m\n    Length                  " << MEM_SIZE  << " bits" << std::endl;
    }

    void Load( Byte ins[256], Word MEM_START ) {

        for ( u32 i = 0; i < 256; i++ ) {
            if ( ins[i] == 0xEF ) {
                std::cout << "    Instruction length      " << i  << " bytes" << std::endl;
                i = 256; break;
            } else { Data[MEM_START+i] = ins[i]; }
        }
    }

    void Dump( u32 start=0x0100, u32 length=256 , string label="" , bool format=1 ) { // Ugly code right here, do not open pls
        std::vector<Byte> range;
        for (u32 i = start; i < start + length; ++i) {
            range.push_back(Data[i]);
        }

        std::cout << "\n\033[1;31m-- Memory Dump --\033[0m [0x" << start << " - 0x" << start + length - 1 << "] " << label << std::endl;

        int l = 0;
        if (format) {

            for (size_t i = 0; i < range.size(); ++i) {

                if ( +range[i] == 0xAF ) {
                    std::cout << "-- ";
                } else {
                    if ( +range[i] < 0x10 ) {
                        std::cout << "0" << hex << +range[i] << " ";
                    } else {
                        std::cout << hex << +range[i] << " ";
                    }
                }
                if ( l >= 31 ) { std::cout << std::endl; l=0; } else { l++; }

            }
        } else {

            for (size_t i = 0; i < range.size(); ++i) {
                if ( +range[i] == 0xAF ) {
                    std::cout << "00000000 ";
                } else {
                    std::cout << bitset<8>(+range[i]) << " ";
                }
                if ( l >= 15 ) { std::cout << std::endl; l=0; } else { l++; }
            }

        }
        std::cout << std::endl;
    }
    
};

struct CPU {

    Word PC;                        // Program Counter
    Byte SP;                        // Stack Pointer

    Byte AC, IX, IY;                // Accumulator, Index X, Index Y
    bitset<8> SR;                   // Status flags: Carry, Zero, Interrupt Disable, Decimal Mode, Break Command, -Unused-, Overflow flag, Negative flag

    void Cycle( u32& cycles , u32 amount=1) {
        for ( amount > 0; amount--; ) {
            cycles--;
            usleep(1000000 / (config.clockSpeed * config.simFactor));   // fabricated clock speed; timeout between cycles
        }
    }

    Byte FetchByte( u32& cycles, Memory& memory ) {
        if ( cycles > 0 ) {
            PC++;
            Byte Data = memory[PC];
            if( config.verbose ) { std::cout << "\n > Fetching Byte from memory at position 0x" << hex << uppercase << PC+0 << " :: 0x" << Data+0 << std::endl; }
            Cycle( cycles );
            return Data;
        } else {
            return 0x00;
        }
    }   

    void Push( u32& cycles, Byte data, Memory& memory) {
        memory[(0x01 << 8) | SP] = data;
        SP--;
        Cycle( cycles );
    }
    void Pull( u32& cycles, Memory& memory ) {
        SP++;
        memory[(0x01 << 8) | SP] = 0x00;
        Cycle( cycles );
    }

    void Dump() {
        std::cout << "\n\033[1;31m-- CPU Dump --\033[0m" << hex << uppercase
        << "\n  PC      0x" << PC + 0
        << "\n  SP      0x" << SP + 0
        << "\n  AC      0x" << AC + 0
        << "\n  X       0x" << IX + 0 << "      Y: 0x" << IY + 0
        << "\n  Flags   0b" << bitset<8>(SR) << " [N,O,-,B,D,I,Z,C]" << std::endl;
    }

    void Reset() {
        PC = 0x01FF;
        SP = 0xFF;
        AC = IX = IY = 0x00;
        SR[2] = 1;
    }

    void Exec( u32 cycles, Memory& memory ) {

        Dump();
        std::cout << "\n > Clock speed: " << config.clockSpeed << "Hz\n > Simulated clock speed: " << config.clockSpeed * config.simFactor << "Hz\n\033[1;31mExecuting instructions =>\033[0m\n" << std::endl;

        while ( cycles > 0 ) {

            Byte Ins = FetchByte( cycles, memory );
            switch ( Ins ) {

                // Accumulator Stuff
                case 0x69: {                                                // ADC - Add to Accumulator with carry (immediate - 2 cycles) - C, Z, V, N
                    Byte Value = FetchByte( cycles, memory );
                    AC += Value + (SR[1] ? 0x01 : 0x00);
                    SR[0] = AC > 255 ? 1 : 0;
                    SR[1] = AC == 0 ? 1 : 0;
                    // Don't understand overflow flag, so SR[6] not implemented
                    // SR[7] = {AC bit 7 on} ? 1 : 0; ??? if zero
                    std::cout << "    ! Ins: ADC [Add with carry] -> 0x" << Value + 0 << std::endl;
                } break;
                case 0x29: {                                                // AND - Binary AND memory with Accumulator (immediate - 2 cycles) - Z, N
                    Byte Value = FetchByte( cycles, memory );
                    AC &= Value;
                    SR[1] = AC == 0 ? 1 : 0;
                    // SR[7] = {AC bit 7 on} ? 1 : 0; ??? if zero
                    std::cout << "    ! Ins: AND [Binary AND] -> 0x" << Value + 0 << std::endl;
                } break;
                case 0xC9: {                                                // CMP - Compare Accumulator (immediate - 2 cycles) - C, Z, N
                    Byte Value = FetchByte( cycles, memory );
                    SR[1] = (AC == Value) ? 1 : 0;
                    SR[0] = (AC >= Value) ? 1 : 0;
                    // SR[7] = {AC bit 7 on} ? 1 : 0; ??? if zero
                    std::cout << "    ! Ins: CPX [Compare Accumulator] -> 0x" << Value + 0 << std::endl;
                } break;

                // Branch Stuff
                case 0x90: {                                                // BCC! - Branch on carry clear (relative - 2 cycles + 1 if page crossed + 1 if branch taken)
                    Byte Value = FetchByte( cycles, memory );
                    if ( SR[0] == 0 ) { PC += Value; cycles--; } else { continue; }
                    std::cout << "    ! Ins: BEQ [Branch on carry clear] -> 0x" << Value + 0 << " => " << (SR[0] == 0) << std::endl;
                } break;
                case 0xB0: {                                                // BCS! - Branch on carry set (relative - 2 cycles + 1 if page crossed + 1 if branch taken)
                    Byte Value = FetchByte( cycles, memory );
                    if ( SR[0] == 1 ) { PC += Value; cycles--; } else { continue; }
                    std::cout << "    ! Ins: BEQ [Branch on carry set] -> 0x" << Value + 0 << " => " << (SR[0] == 1) << std::endl;
                } break;
                case 0xF0: {                                                // BEQ! - Branch on zero set (relative - 2 cycles + 1 if page crossed + 1 if branch taken)
                    Byte Value = FetchByte( cycles, memory );
                    if ( SR[1] == 1 ) { PC += Value; cycles--; } else { continue; }
                    std::cout << "    ! Ins: BEQ [Branch on zero set] -> 0x" << Value + 0 << " => " << (SR[1] == 1) << std::endl;
                } break;
                case 0x30: {                                                // BMI! - Branch on negative set (relative - 2 cycles + 1 if page crossed + 1 if branch taken)
                    Byte Value = FetchByte( cycles, memory );
                    if ( SR[7] == 1 ) { PC += Value; cycles--; } else { continue; }
                    std::cout << "    ! Ins: BMI [Branch on negative set] -> 0x" << Value + 0 << " => " << (SR[7] == 1) << std::endl;
                } break;
                case 0xD0: {                                                // BNE! - Branch on zero clear (relative - 2 cycles + 1 if page crossed + 1 if branch taken)
                    Byte Value = FetchByte( cycles, memory );
                    if ( SR[1] == 0 ) { PC += Value; cycles--; } else { continue; }
                    std::cout << "    ! Ins: BNE [Branch on zero clear] -> 0x" << Value + 0 << " => " << (SR[1] == 0) << std::endl;
                } break;
                case 0x10: {                                                // BPL! - Branch on negative clear (relative - 2 cycles + 1 if page crossed + 1 if branch taken)
                    Byte Value = FetchByte( cycles, memory );
                    if ( SR[7] == 0 ) { PC += Value; cycles--; } else { continue; }
                    std::cout << "    ! Ins: BPL [Branch on negative clear] -> 0x" << Value + 0 << " => " << (SR[7] == 0) << std::endl;
                } break;
                case 0x50: {                                                // BVC! - Branch on overflow clear (relative - 2 cycles + 1 if page crossed + 1 if branch taken)
                    Byte Value = FetchByte( cycles, memory );
                    if ( SR[6] == 0 ) { PC += Value; cycles--; } else { continue; }
                    std::cout << "    ! Ins: BVC [Branch on overflow clear] -> 0x" << Value + 0 << " => " << (SR[6] == 0) << std::endl;
                } break;
                case 0x70: {                                                // BVS! - Branch on overflow set (relative - 2 cycles + 1 if page crossed + 1 if branch taken)
                    Byte Value = FetchByte( cycles, memory );
                    if ( SR[6] == 1 ) { PC += Value; cycles--; } else { continue; }
                    std::cout << "    ! Ins: BVS [Branch on overflow set] -> 0x" << Value + 0 << " => " << (SR[6] == 1) << std::endl;
                } break;

                // Flag stuff
                case 0x18: {                                                // CLC! - clear carry flag (implied - 2 cycles) - C
                    SR[0] = 0; cycles--;
                    std::cout << "    ! Ins: CLC [Clear carry flag]" << std::endl;
                } break;
                case 0xD8: {                                                // CLD! - clear decimal flag (implied - 2 cycles) - D
                    SR[4] = 0; cycles--;
                    std::cout << "    ! Ins: CLD [Clear decimal flag]" << std::endl;
                } break;
                case 0x58: {                                                // CLI! - clear interrupt disable flag (implied - 2 cycles) - I
                    SR[3] = 0; cycles--;
                    std::cout << "    ! Ins: CLI [Clear interrupt disable flag]" << std::endl;
                } break;

                // All the following instructions are really shit and have to be revised/extended

                case 0xA9: {                                                // LDA - Load Accumulator (2 cycles)
                    Byte Value = FetchByte( cycles, memory );
                    AC = Value;
                    SR[1] = (AC == 0) ? 1 : 0;
                    SR[7] = 0; // set if bit 7 of AC is set
                    std::cout << "    ! Ins: LDA [Load Accumulator] -> 0x" << Value + 0 << std::endl;
                } break;

                case 0xAA: {                                                // TAX! - TranSRer Accumulator to X (2 cycles)
                    std::cout << "    ! Ins: TAX [Copy Accumulator to X]" << std::endl;
                    IX = AC;
                    Cycle( cycles );
                } break;

                case 0x4C: {                                                // JMP - Jump (3 cycles)
                    Byte Value1 = FetchByte( cycles, memory );
                    Byte Value2 = FetchByte( cycles, memory );
                    PC = (Value2 << 8) | Value1;
                    std::cout << "    ! Ins: JMP [Jump to] -> 0x" << PC + 0 << std::endl;
                } break;

                case 0x48: {                                                // PHA - Push Accumulator to Stack (3 cycles)
                    std::cout << "    ! Ins: PHA [Push Accumulator]" << std::endl;
                    Push(cycles, AC, memory);
                    Cycle( cycles, 2 );
                } break;

                case 0xCA: {                                                // DEX - Decrement X (2 cycles)
                    IX--;
                    std::cout << "    ! Ins: DEX [Decrement X]" << std::endl;
                } break;

                case 0xEA: {                                                // NOP
                    Cycle( cycles );
                } break;

                case 0xEF: {                                                // Exit
                    std::cout << "    ! Ins: Exit" << std::endl;
                    Dump();
                    cycles = 0;
                } break;

                default: {
                    std::cout << "    ! ERR: no instruction identified: " << Ins+0 << std::endl;
                    cycles = 0;
                } break;

            }

        }
    }

};

int main() {
    std::cout << "\033[1;31m\n  | | | | | | | | | | | | | | | | | | | | | | |\n-------------------------------------------------\n|                                               |\n|                 6502 Emulator                 |\n|                                               |\n-------------------------------------------------\n  | | | | | | | | | | | | | | | | | | | | | | |\n\033[0m" << std::endl; 
    
    config.verbose = false;
    config.simFactor = 0.01;

    Byte Ins[] = {
        0xA9, 0xAB,                 // Load Accumulator with 0xAB
        0xAA,                       // Transfer Accumulator to X
        0x48,                       // Push Accumulator
        0x69, 0x1D,                 // Add 0x1D to Accumulator
        0xC9, 0xC8,                 // Compare Accumulator to 0xC8
        0xF0, 0x02,                 // Branch if zero set
        0x48,                       // Push Accumulator
        0xEA,                       // NOP
        0x4C, 0xF9, 0xFF,           // Jump to 0xFFFA (exit)
        0xEF,
    };
    
    Memory mem;
    CPU cpu;

    mem.Init( 0x00 );                           // Initialise memory (with 0xAF to suppress 0's)
    mem[0xFFFA] = 0xEF;                         // Set code exit address
    mem.Load(Ins, 0x0200);                      // Load instructions into memory

    cpu.Reset();                                // Reset cpu
    cpu.Exec( config.maxCycles, mem );          // Execute instructions

    mem.Dump(0x0180, 128, "-- Stack --");
    mem.Dump(0x0200, 128);

    return 0;
};
