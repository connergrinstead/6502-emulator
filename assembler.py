from time import sleep

# This is a kinda janky assembler for the 6502 emulator
# It includes all 13 addressing modes for all 151 documented opcodes

#ins = "LDA #$AB TAX PHA ADC #$1D CMP #$C8 BEQ $02 PHA NOP JMP $FFF9 EXT"


addrSet = {
    "n": "acc",
    "#$nn": "imm",
    "$nnnn": "abs",
    "$nnnn,X": "x-abs",
    "$nnnn,Y": "y-abs",
    "($nnnn)": "abs-ind",
    "$nn": "zer",
    "$nn,X": "x-zer",
    "$nn,Y": "y-zer",
    "($nn,X)": "x-zer-ind",
    "($nn,Y)": "y-zer-ind",
}

insSet = {
    "EXT": { "imp": "EF" },
    "NOP": { "imp": "EA" },

    # Load Instructions
    "LDA": { "imm": "A9", "abs": "AD", "x-abs": "BD", "y-abs": "B9", "zer": "A5", "x-zer": "B5", "x-zer-ind": "A1", "y-zer-ind": "B1", },
    "LDX": { "imm": "A2", "abs": "AE", "y-abs": "BE", "zer": "A6", "y-zer": "B6", },
    "LDY": { "imm": "A2", "abs": "AE", "x-abs": "BE", "zer": "A6", "x-zer": "B6", },
    "STA": { "abs": "8D", "x-abs": "9D", "y-abs": "99", "zer": "85", "x-zer": "95", "x-zer-ind": "81", "y-zer-ind": "91", },
    "STX": { "abs": "8E", "zer": "86", "x-zer": "94", },
    "STY": { "abs": "8C", "zer": "84", "x-zer": "94", },

    # Transfer Instructions
    "TAX": { "imp": "AA" },
    "TAY": { "imp": "A8" },
    "TSX": { "imp": "BA" },
    "TXA": { "imp": "8A" },
    "TXS": { "imp": "9A" },
    "TYA": { "imp": "98" },

    # Stack Instructions
    "PHA": { "imp": "48" },
    "PHP": { "imp": "08" },
    "PLA": { "imp": "68" },
    "PLP": { "imp": "28" },

    # Shift Instructions
    "ASL": { "acc": "0A", "abs": "0E", "x-abs": "1E", "zer": "06", "x-zer": "16", },
    "LSR": { "acc": "4A", "abs": "4E", "x-abs": "5E", "zer": "46", "x-zer": "56", },
    "ROL": { "acc": "2A", "abs": "2E", "x-abs": "3E", "zer": "26", "x-zer": "36", },
    "ROR": { "acc": "6A", "abs": "6E", "x-abs": "7E", "zer": "66", "x-zer": "76", },

    # Logic Instructions
    "AND": { "imm": "29", "abs": "2D", "x-abs": "3D", "y-abs": "39", "zer": "25", "x-zer": "35", "x-zer-ind": "21", "y-zer-ind": "31", },
    "LSR": { "acc": "4A", "abs": "4E", "x-abs": "5E", "zer": "46", "x-zer": "56", },
    "ROL": { "acc": "2A", "abs": "2E", "x-abs": "3E", "zer": "26", "x-zer": "36", },
    "ROR": { "acc": "6A", "abs": "6E", "x-abs": "7E", "zer": "66", "x-zer": "76", },

    # Arithmetic Instructions
    "ADC": { "imm": "69", "abs": "6D", "x-abs": "7D", "y-abs": "79", "zer": "65", "x-zer": "75", "x-zer-ind": "61", "y-zer-ind": "71", },
    "CMP": { "imm": "C9", "abs": "CD", "x-abs": "DD", "y-abs": "D9", "zer": "C5", "x-zer": "D5", "x-zer-ind": "C1", "y-zer-ind": "D1", },
    "CPX": { "imm": "E0", "abs": "EC", "zer": "E4", },
    "CPY": { "imm": "C0", "abs": "CC", "zer": "C4", },
    "SBC": { "imm": "E9", "abs": "ED", "x-abs": "FD", "y-abs": "F9", "zer": "E5", "x-zer": "F5", "x-zer-ind": "E1", "y-zer-ind": "F1", },

    # Increment/Decrement Instructions
    "DEC": { "abs": "CE", "x-abs": "DE", "zer": "C6", "x-zer": "D6", },
    "DEX": { "imp": "CA" },
    "DEY": { "imp": "88" },
    "INC": { "abs": "EE", "x-abs": "FE", "zer": "E6", "x-zer": "F6", },
    "INX": { "imp": "E8" },
    "INY": { "imp": "C8" },

    # Control Instructions
    "BRK": { "imp": "00" },
    "JMP": { "abs": "4C", "abs-ind": "6C", },
    "JSR": { "abs": "20" },
    "RTI": { "imp": "40" },
    "RTS": { "imp": "60" },

    # Branch Instructions
    "BCC": { "rel": "90" },
    "BCS": { "rel": "B0" },
    "BEQ": { "rel": "F0" },
    "BMI": { "rel": "30" },
    "BNE": { "rel": "D0" },
    "BPL": { "rel": "10" },
    "BVC": { "rel": "50" },
    "BVS": { "rel": "70" },

    # Flag Instructions
    "CLC": { "imp": "18" },
    "CLD": { "imp": "D8" },
    "CLI": { "imp": "58" },
    "CLV": { "imp": "B8" },
    "SEC": { "imp": "38" },
    "SED": { "imp": "F8" },
    "SEI": { "imp": "78" },
}

# Get Addressing Type
def AddrType(operand):
    for char in ["1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"]:
        if char in operand:
            operand = operand.replace(char, 'n')
    print(" [] - Operand abstracted as: " + operand)
    if operand in addrSet:
        print(" [] - Addressing Type match: " + addrSet[operand])
        return(addrSet[operand])
    else: print("\n! Incorrect Addressing Type format: " + operand); return 0

# Build binary (in hex :D)
def Exec():

    code = ""
    i = 0
    while i < len(insList):
        sleep(.15)

        if insList[i] not in insSet: print("\n! Unsupported Operation at i == " + str(i) + ": " + insList[i]); return 0

        print(f"\nAssembling at i == {str(i)}: {insList[i]}")

        # If non implied addressing type (= with operand):
        if insList[i] not in ["BRK", "CLC", "CLD", "CLI", "CLV", "DEX", "DEY", "INX", "INY", "NOP", "PHA", "PHP", "PLA", "PLP",
                                "RTI", "RTS", "SEC", "SED", "SEI", "TAX", "TAY", "TSX", "TXA", "TXS", "TYA", "EXT"]:
            
            # If branch instruction:
            if insList[i] in ["BCC", "BCS", "BEQ", "BMI", "BNE", "BPL", "BVC", "BVS"]:
                addrType = "rel"
            else:
                addrType = AddrType(insList[i+1])

            try:
                opcode = insSet[insList[i]][addrType]
            except: print("\n! Non-compatible Addressing Type at i == " + str(i) + ": " + insList[i]); return 0

            print(f" - Addressing type == {addrType} and opcode == {opcode}")

            if addrType == "imm":
                code += f"0x{opcode}, 0x{insList[i+1][2:4]}, "
                print(f" -> 0x{opcode}, 0x{insList[i+1][2:4]}, ")
                i+=1

            elif addrType == "abs" or addrType == "x-abs" or addrType == "y-abs":
                code += f"0x{opcode}, 0x{insList[i+1][3:5]}, 0x{insList[i+1][1:3]}, "
                print(f" -> 0x{opcode}, 0x{insList[i+1][3:5]}, 0x{insList[i+1][1:3]}, ")
                i+=1

            elif addrType == "abs-ind":
                code += f"0x{opcode}, 0x{insList[i+1][4:6]}, 0x{insList[i+1][2:4]}, "
                print(f" -> 0x{opcode}, 0x{insList[i+1][4:6]}, 0x{insList[i+1][2:4]}, ")
                i+=1

            elif addrType == "zer" or addrType == "x-zer" or addrType == "y-zer":
                code += f"0x{opcode}, 0x{insList[i+1][1:3]}, "
                print(f" -> 0x{opcode}, 0x{insList[i+1][1:3]}, ")
                i+=1

            elif addrType == "y-zer-ind" or addrType == "x-zer-ind":
                code += f"0x{opcode}, 0x{insList[i+1][2:4]}, "
                print(f" -> 0x{opcode}, 0x{insList[i+1][2:4]}, ")
                i+=1

            elif addrType == "acc":
                code += f"0x{opcode}, "
                print(f" -> 0x{opcode}, ")
                i+=1
            
            # Branch exclusive Addressing Type
            elif addrType == "rel":
                code += "branch, "
                print("branch ")
                i+=1

        else:
            opcode = insSet[insList[i]]["imp"]
            print(" - Addressing type == imp and Opcode == " + str(opcode))
            code += f"0x{opcode}, "
            print(f" -> 0x{opcode}, ")

        i+=1

    return code

ins = "LDA #$AB TAX PHA ADC #$1D CMP #$C8 BEQ $02 PHA NOP JMP $FFF9"

insList = list(ins.replace('\n', ' ').split(" "))

print("\033[1;34m\n  | | | | | | | | | | | | | | | | | | | | | | |\n-------------------------------------------------\n|                                               |\n|                 6502 Assembler                |\n|                                               |\n-------------------------------------------------\n  | | | | | | | | | | | | | | | | | | | | | | |\n\033[0m")
print("\nAssembling instructions:\n" + ins)
print(f"\n\n{ins}\n-->\n{Exec()}0xEF")
