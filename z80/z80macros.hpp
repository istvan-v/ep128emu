/*
 *  ENTER emulator (c) Copyright, Kevin Thacker 1995-2001
 *
 *  This file is part of the ENTER emulator source code distribution.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */

#include "z80.hpp"

#define RD_BYTE_INDEX()         readMemory(R.IndexPlusOffset)
#define WR_BYTE_INDEX(Data)     writeMemory(R.IndexPlusOffset, (Data))

#define INC_REFRESH(Count)      R.R += (Count)

#define SETUP_INDEXED_ADDRESS(Index)            \
        R.IndexPlusOffset = (Index) + (Z80_BYTE_OFFSET) readOpcodeByte(2)

/* overflow caused, when both are + or -, and result is different. */
#define SET_OVERFLOW_FLAG_A_ADD(Reg, Result)                            \
{                                                                       \
        Z80_BYTE  tmpByte_ = (Result);                                  \
        Z80_FLAGS_REG &= (0xFF ^ Z80_PARITY_FLAG);                      \
        Z80_FLAGS_REG |= (((R.AF.B.h ^ tmpByte_) & ((Reg) ^ tmpByte_) & 0x80) \
                          >> (7 - Z80_PARITY_FLAG_BIT));                \
}

#define SET_OVERFLOW_FLAG_A_SUB(Reg, Result)                            \
        Z80_FLAGS_REG &= (0xFF ^ Z80_PARITY_FLAG);                      \
        Z80_FLAGS_REG |= ((((Reg) ^ R.AF.B.h) & ((Result) ^ R.AF.B.h) & 0x80) \
                          >> (7 - Z80_PARITY_FLAG_BIT))

#define SET_HALFCARRY(Reg, Result)                                      \
        Z80_FLAGS_REG &= (0xFF ^ Z80_HALFCARRY_FLAG);                   \
        Z80_FLAGS_REG |= (((Reg) ^ R.AF.B.h ^ (Result)) & Z80_HALFCARRY_FLAG)

/* halfcarry carry out of bit 11 */
#define SET_HALFCARRY_16(Reg1, Reg2, Result)                            \
        Z80_FLAGS_REG &= (0xFF ^ Z80_HALFCARRY_FLAG);                   \
        Z80_FLAGS_REG |= ((((Reg1)^(Reg2)^(Result)) >> 8) & Z80_HALFCARRY_FLAG)

#define SET_OVERFLOW_FLAG_HL_ADD(Reg, Result)                           \
{                                                                       \
        Z80_WORD  tmpWord_ = (Result);                                  \
        Z80_FLAGS_REG &= (0xFF ^ Z80_PARITY_FLAG);                      \
        Z80_FLAGS_REG |= (((R.HL.W ^ tmpWord_) & ((Reg) ^ tmpWord_) & 0x8000) \
                          >> (15 - Z80_PARITY_FLAG_BIT));               \
}

#define SET_OVERFLOW_FLAG_HL_SUB(Reg, Result)                           \
        Z80_FLAGS_REG &= (0xFF ^ Z80_PARITY_FLAG);                      \
        Z80_FLAGS_REG |= ((((Reg) ^ R.HL.W) & ((Result) ^ R.HL.W) & 0x8000) \
                          >> (15 - Z80_PARITY_FLAG_BIT))

#define SET_ZERO_FLAG(Register)                                         \
{                                                                       \
    Z80_FLAGS_REG = Z80_FLAGS_REG & (0xFF ^ Z80_ZERO_FLAG);             \
    if ((Register) == 0)                                                \
        Z80_FLAGS_REG |= Z80_ZERO_FLAG;                                 \
}

#define SET_ZERO_FLAG16(Register)                                       \
{                                                                       \
    Z80_FLAGS_REG &= (0xFF ^ Z80_ZERO_FLAG);                            \
    if (((Register) & 0xFFFF) == 0)                                     \
        Z80_FLAGS_REG |= Z80_ZERO_FLAG;                                 \
}

#define SET_SIGN_FLAG(Register)                                         \
{                                                                       \
    Z80_FLAGS_REG = Z80_FLAGS_REG & (0xFF ^ Z80_SIGN_FLAG);             \
    Z80_FLAGS_REG = Z80_FLAGS_REG | ((Register) & 0x80);                \
}

#define SET_SIGN_FLAG16(Register)                                       \
{                                                                       \
    Z80_FLAGS_REG = Z80_FLAGS_REG & (0xFF ^ Z80_SIGN_FLAG);             \
    Z80_FLAGS_REG = Z80_FLAGS_REG | (((Register) & 0x8000) >> 8);       \
}

#define SET_CARRY_FLAG_ADD(Result)                                      \
{                                                                       \
    Z80_FLAGS_REG = Z80_FLAGS_REG & (0xFF ^ Z80_CARRY_FLAG);            \
    Z80_FLAGS_REG = Z80_FLAGS_REG | (((Result) >> 8) & 0x01);           \
}

#define SET_CARRY_FLAG_ADD16(Result)                                    \
{                                                                       \
    Z80_FLAGS_REG = (Z80_BYTE) (Z80_FLAGS_REG & (0xFF ^ Z80_CARRY_FLAG)); \
    Z80_FLAGS_REG = (Z80_BYTE) (Z80_FLAGS_REG | (((Result) >> 16) & 0x01)); \
}

#define SET_CARRY_FLAG_SUB(Result)                                      \
{                                                                       \
    Z80_FLAGS_REG = Z80_FLAGS_REG & (0xFF ^ Z80_CARRY_FLAG);            \
    Z80_FLAGS_REG = Z80_FLAGS_REG | (((Result) >> 8) & 0x01);           \
}

#define SET_CARRY_FLAG_SUB16(Result)                                    \
{                                                                       \
    Z80_FLAGS_REG = (Z80_BYTE) (Z80_FLAGS_REG & (0xFF ^ Z80_CARRY_FLAG)); \
    Z80_FLAGS_REG = (Z80_BYTE) (Z80_FLAGS_REG | (((Result) >> 16) & 0x01)); \
}

/* set zero and sign flag */
#define SET_ZERO_SIGN(Register)                                         \
{                                                                       \
    Z80_FLAGS_REG = Z80_FLAGS_REG & (0xFF ^ (Z80_ZERO_FLAG | Z80_SIGN_FLAG)); \
    Z80_FLAGS_REG = Z80_FLAGS_REG | t.zeroSignTable[(Register) & 0xFF]; \
}

/* set zero, sign and parity flag */
#define SET_ZERO_SIGN_PARITY(Register)                                  \
{                                                                       \
    Z80_FLAGS_REG = (Z80_FLAGS_REG & (Z80_HALFCARRY_FLAG                \
                                      | Z80_SUBTRACT_FLAG | Z80_CARRY_FLAG)) \
                    | t.zeroSignParityTable[(Register) & 0xFF];         \
}

#define LD_R_n(Register)        Register = readOpcodeByte(1)

#define LD_RI_n(Register)       Register = readOpcodeByte(2)

#define LD_R_HL(Register)       Register = readMemory(R.HL.W)

#define LD_R_INDEX(Index, Register)     Register = RD_BYTE_INDEX_(Index)

#define LD_INDEX_R(Index, Register)     WR_BYTE_INDEX_(Index, (Register))

#define LD_HL_R(Register)       writeMemory(R.HL.W, (Register))

#define LD_A_RR(Register)       R.AF.B.h = readMemory(Register)

#define LD_RR_A(Register)       writeMemory((Register), R.AF.B.h)

#define LD_INDEX_n(Index)                                               \
{                                                                       \
        SETUP_INDEXED_ADDRESS(Index);                                   \
        Z80_BYTE  tempByte = readOpcodeByte(3);                         \
        updateCycles(2);                                                \
        WR_BYTE_INDEX(tempByte);                                        \
}

/*-----*/
/* RES */

#define RES(AndMask, Register)          Register = Register & (~AndMask)

#define RES_REG(AndMask, Register)      RES(AndMask, Register)

#define RES_HL(AndMask)                                                 \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        RES(AndMask, tempByte);                                         \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define RES_INDEX(AndMask)                                              \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RES(AndMask, tempByte);                                         \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/* SET */

#define SET(OrMask, Register)           Register = Register | OrMask

#define SET_REG(OrMask, Register)       SET(OrMask, Register)

#define SET_HL(OrMask)                                                  \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        SET(OrMask, tempByte);                                          \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define SET_INDEX(OrMask)                                               \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SET(OrMask, tempByte);                                          \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/* BIT */
#define BIT(BitIndex,Register)                                          \
{                                                                       \
    Z80_BYTE  tempByte = Z80_BYTE(Register) & Z80_BYTE(1 << (BitIndex)); \
    Z80_FLAGS_REG = (Z80_FLAGS_REG & Z80_CARRY_FLAG) | Z80_HALFCARRY_FLAG \
                    | t.zeroSignParityTable[tempByte];                  \
}

#define BIT_REG(BitIndex, Register)     BIT(BitIndex, (Register))

#define BIT_HL(BitIndex)                                                \
{                                                                       \
        BIT(BitIndex, readMemory(R.HL.W));                              \
        updateCycle();                                                  \
}

#define BIT_INDEX(BitIndex)                                             \
{                                                                       \
        BIT(BitIndex, RD_BYTE_INDEX());                                 \
        Z80_FLAGS_REG = (Z80_FLAGS_REG                                  \
                         & (~(Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2)))    \
                        | (Z80_BYTE(R.IndexPlusOffset >> 8)             \
                           & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2));    \
        updateCycle();                                                  \
}

/*------------------*/

#define SHIFTING_FLAGS(Register)                                        \
        SET_ZERO_SIGN_PARITY(Register)                                  \
        Z80_FLAGS_REG &= (~(Z80_HALFCARRY_FLAG | Z80_SUBTRACT_FLAG))

#define SET_CARRY_LEFT_SHIFT(Register)                                  \
        Z80_FLAGS_REG = Z80_FLAGS_REG & (0xFF ^ Z80_CARRY_FLAG);        \
        Z80_FLAGS_REG = Z80_FLAGS_REG | (((Register) >> 7) & 0x01)

#define SET_CARRY_RIGHT_SHIFT(Register)                                 \
        Z80_FLAGS_REG = Z80_FLAGS_REG & (0xFF ^ Z80_CARRY_FLAG);        \
        Z80_FLAGS_REG = Z80_FLAGS_REG | ((Register) & 0x01)

#define RL(Register)                                                    \
{                                                                       \
        Z80_BYTE  Carry = (Z80_FLAGS_REG & (Z80_BYTE) Z80_CARRY_FLAG);  \
        SET_CARRY_LEFT_SHIFT(Register);                                 \
        Register = (Register << 1) | Carry;                             \
}

#define RL_WITH_FLAGS(Register)                                         \
{                                                                       \
        RL(Register);                                                   \
        SHIFTING_FLAGS(Register);                                       \
}

#define RL_REG(Register)        RL_WITH_FLAGS(Register)

#define RL_HL()                                                         \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        RL_WITH_FLAGS(tempByte);                                        \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define RL_INDEX()                                                      \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RL_WITH_FLAGS(tempByte);                                        \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define RR(Register)                                                    \
{                                                                       \
        Z80_BYTE  Carry = Z80_FLAGS_REG & Z80_CARRY_FLAG;               \
        SET_CARRY_RIGHT_SHIFT(Register);                                \
        Register = (Register >> 1) | (Carry << 7);                      \
}

#define RR_WITH_FLAGS(Register)                                         \
{                                                                       \
        RR(Register);                                                   \
        SHIFTING_FLAGS(Register);                                       \
}

#define RR_REG(Register)        RR_WITH_FLAGS(Register)

#define RR_HL()                                                         \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        RR_WITH_FLAGS(tempByte);                                        \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define RR_INDEX()                                                      \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RR_WITH_FLAGS(tempByte);                                        \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/* RLCA doesn't set sign or zero! */

#define RLC(Register)                                                   \
{                                                                       \
        SET_CARRY_LEFT_SHIFT(Register);                                 \
        Register = (Register << 1) | ((Register >> 7) & 0x01);          \
}

#define RLC_WITH_FLAGS(Register)                                        \
{                                                                       \
        RLC(Register);                                                  \
        SHIFTING_FLAGS(Register);                                       \
}

#define RLC_REG(Register)       RLC_WITH_FLAGS(Register)

#define RLC_HL()                                                        \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        RLC_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define RLC_INDEX()                                                     \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RLC_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define RRC(Register)                                                   \
{                                                                       \
        SET_CARRY_RIGHT_SHIFT(Register);                                \
        Register = (Register >> 1) | ((Register & 0x01) << 7);          \
}

#define RRC_WITH_FLAGS(Register)                                        \
{                                                                       \
        RRC(Register);                                                  \
        SHIFTING_FLAGS(Register);                                       \
}

#define RRC_REG(Register)       RRC_WITH_FLAGS(Register)

#define RRC_HL()                                                        \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        RRC_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define RRC_INDEX()                                                     \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RRC_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define SLA(Register)                                                   \
{                                                                       \
        Z80_BYTE  Reg = Register;                                       \
        Z80_BYTE  Flags = ((Reg >> 7) & 0x01);      /* carry */         \
        Reg = (Reg << 1);                                               \
        /* sign, zero, parity, f5, f3 */                                \
        Z80_FLAGS_REG = Flags | t.zeroSignParityTable[Reg];             \
        Register = Reg;                                                 \
}

#define SLA_WITH_FLAGS(Register)        SLA(Register)

#define SLA_REG(Register)               SLA_WITH_FLAGS(Register)

#define SLA_HL()                                                        \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        SLA_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define SLA_INDEX()                                                     \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SLA_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define SRA(Register)                                                   \
{                                                                       \
        Z80_BYTE  Reg = Register;                                       \
        Z80_BYTE  Flags = Reg & 0x01;               /* carry */         \
        Reg = (Reg >> 1) | (Reg & 0x80);                                \
        /* sign, zero, parity, f5, f3 */                                \
        Z80_FLAGS_REG = Flags | t.zeroSignParityTable[Reg];             \
        Register = Reg;                                                 \
}

#define SRA_WITH_FLAGS(Register)        SRA(Register)

#define SRA_REG(Register)               SRA_WITH_FLAGS(Register)

#define SRA_HL()                                                        \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        SRA_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define SRA_INDEX()                                                     \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SRA_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define SRL(Register)                                                   \
{                                                                       \
        Z80_BYTE  Reg = Register;                                       \
        Z80_BYTE  Flags = Reg & 0x01;               /* carry */         \
        Reg = Reg >> 1;                                                 \
        /* sign, zero, parity, f5, f3 */                                \
        Z80_FLAGS_REG = Flags | t.zeroSignParityTable[Reg];             \
        Register = Reg;                                                 \
}

#define SRL_WITH_FLAGS(Register)        SRL(Register)

#define SRL_REG(Register)               SRL_WITH_FLAGS(Register)

#define SRL_HL()                                                        \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        SRL_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define SRL_INDEX()                                                     \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SRL_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define SLL(Register)                                                   \
{                                                                       \
        Z80_BYTE  Reg = Register;                                       \
        Z80_BYTE  Flags = (Reg >> 7) & 0x01;        /* carry */         \
        Reg = ((Reg << 1) | 0x01);                                      \
        /* sign, zero, parity, f5, f3 */                                \
        Z80_FLAGS_REG = Flags | t.zeroSignParityTable[Reg];             \
        Register = Reg;                                                 \
}

#define SLL_WITH_FLAGS(Register)        SLL(Register)

#define SLL_REG(Register)               SLL_WITH_FLAGS(Register)

#define SLL_HL()                                                        \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        SLL_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define SLL_INDEX()                                                     \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SLL_WITH_FLAGS(tempByte);                                       \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/*---------------*/

#define SET_LOGIC_FLAGS                                                 \
        SET_ZERO_SIGN_PARITY(R.AF.B.h);                                 \
        Z80_FLAGS_REG = Z80_FLAGS_REG & (Z80_ZERO_FLAG | Z80_PARITY_FLAG | Z80_SIGN_FLAG)

#define AND_A_X(Register)                                               \
{                                                                       \
        R.AF.B.h = R.AF.B.h & (Register);                               \
        Z80_FLAGS_REG = t.zeroSignParityTable[R.AF.B.h] | Z80_HALFCARRY_FLAG; \
}

#define AND_A_R(Register)       AND_A_X(Register)

#define AND_A_INDEX(Index)      AND_A_X(RD_BYTE_INDEX_(Index))

#define XOR_A_X(Register)                                               \
{                                                                       \
        R.AF.B.h = R.AF.B.h ^ (Register);                               \
        Z80_FLAGS_REG = t.zeroSignParityTable[R.AF.B.h];                \
}

#define XOR_A_R(Register)       XOR_A_X(Register)

#define XOR_A_INDEX(Index)      XOR_A_X(RD_BYTE_INDEX_(Index))

#define OR_A_X(Register)                                                \
{                                                                       \
        R.AF.B.h = R.AF.B.h | (Register);                               \
        Z80_FLAGS_REG = t.zeroSignParityTable[R.AF.B.h];                \
}

#define OR_A_R(Register)        OR_A_X(Register)

#define OR_A_INDEX(Index)       OR_A_X(RD_BYTE_INDEX_(Index))

/*-----*/
/* INC */

#define INC_X(Register)                                                 \
{                                                                       \
        Z80_BYTE  Flags = Z80_FLAGS_REG & Z80_CARRY_FLAG;               \
        Register++;                                                     \
        if ((Register & 0x0F) == 0)                                     \
        {                                                               \
                Flags |= Z80_HALFCARRY_FLAG;                            \
                if ((Z80_BYTE) Register == (Z80_BYTE) 0x80)             \
                        Flags |= Z80_OVERFLOW_FLAG;                     \
        }                                                               \
        Flags |= t.zeroSignTable[Register];                             \
        Flags |= Register & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);      \
        Z80_FLAGS_REG = Flags;                                          \
}

#define INC_R(Register)         INC_X(Register)

#define INC_HL_()                                                       \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        INC_X(tempByte);                                                \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define _INC_INDEX_(Index)                                              \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX_(Index);                     \
        INC_X(tempByte);                                                \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/* DEC */
#define DEC_X(Register)                                                 \
{                                                                       \
        Z80_BYTE  Flags =                                               \
                (Z80_FLAGS_REG & Z80_CARRY_FLAG) | Z80_SUBTRACT_FLAG;   \
        if ((Register & 0x0F) == 0)                                     \
        {                                                               \
                Flags |= Z80_HALFCARRY_FLAG;                            \
                if ((Z80_BYTE) Register == (Z80_BYTE) 0x80)             \
                        Flags |= Z80_OVERFLOW_FLAG;                     \
        }                                                               \
        Register--;                                                     \
        Flags |= t.zeroSignTable[Register];                             \
        Flags |= Register & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);      \
        Z80_FLAGS_REG = Flags;                                          \
}

#define DEC_R(Register)         DEC_X(Register)

#define DEC_HL_()                                                       \
{                                                                       \
        Z80_BYTE  tempByte = readMemory(R.HL.W);                        \
        DEC_X(tempByte);                                                \
        updateCycle();                                                  \
        writeMemory(R.HL.W, tempByte);                                  \
}

#define _DEC_INDEX_(Index)                                              \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX_(Index);                     \
        DEC_X(tempByte);                                                \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/*-----------------*/

#define LD_RR_nn(Register)      Register = readOpcodeWord(1)

#define LD_INDEXRR_nn(Index)    Index = readOpcodeWord(2)

#define LD_INDEXRR_nnnn(Index)  Index = readMemoryWord(readOpcodeWord(2))

#define LD_nnnn_INDEXRR(Index)  writeMemoryWord(readOpcodeWord(2), (Index))

#define LD_RR_nnnn(Register)    Register = readMemoryWord(readOpcodeWord(2))

#define LD_nnnn_RR(Register)    writeMemoryWord(readOpcodeWord(2), (Register))

/*--------*/
/* Macros */

#define ADD_A_X(Register2)                                              \
{                                                                       \
        Z80_BYTE  Reg2 = (Register2);                                   \
        Z80_WORD  Result = Z80_WORD(R.AF.B.h) + Z80_WORD(Reg2);         \
        SET_OVERFLOW_FLAG_A_ADD(Reg2, Result);                          \
        SET_CARRY_FLAG_ADD(Result);                                     \
        SET_HALFCARRY(Reg2, Result);                                    \
        R.AF.B.h = Z80_BYTE(Result);                                    \
        R.AF.B.l = (R.AF.B.l & (Z80_HALFCARRY_FLAG | Z80_OVERFLOW_FLAG  \
                                | Z80_CARRY_FLAG))                      \
                   | t.zeroSignTable2[R.AF.B.h];                        \
}

#define ADD_A_R(Register)       ADD_A_X(Register)

#define ADC_A_X(Register)                                               \
{                                                                       \
        Z80_BYTE  Reg = (Register);                                     \
        Z80_WORD  Result = Z80_WORD(R.AF.B.h) + Z80_WORD(Reg)           \
                           + Z80_WORD(Z80_FLAGS_REG & Z80_CARRY_FLAG);  \
        SET_OVERFLOW_FLAG_A_ADD(Reg, Result);                           \
        SET_CARRY_FLAG_ADD(Result);                                     \
        SET_HALFCARRY(Reg, Result);                                     \
        R.AF.B.h = Z80_BYTE(Result);                                    \
        R.AF.B.l = (R.AF.B.l & (Z80_HALFCARRY_FLAG | Z80_OVERFLOW_FLAG  \
                                | Z80_CARRY_FLAG))                      \
                   | t.zeroSignTable2[R.AF.B.h];                        \
}

#define ADC_A_R(Register)       ADC_A_X(Register)

#define SUB_A_X(Register)                                               \
{                                                                       \
        Z80_BYTE  Reg = (Register);                                     \
        Z80_WORD  Result = Z80_WORD(R.AF.B.h) - Z80_WORD(Reg);          \
        SET_OVERFLOW_FLAG_A_SUB(Reg, Result);                           \
        SET_CARRY_FLAG_SUB(Result);                                     \
        SET_HALFCARRY(Reg, Result);                                     \
        R.AF.B.h = Z80_BYTE(Result);                                    \
        R.AF.B.l = (R.AF.B.l & (Z80_HALFCARRY_FLAG | Z80_OVERFLOW_FLAG  \
                                | Z80_CARRY_FLAG))                      \
                   | Z80_SUBTRACT_FLAG | t.zeroSignTable2[R.AF.B.h];    \
}

#define SUB_A_R(Register)       SUB_A_X(Register)

#define CP_A_X(Register)                                                \
{                                                                       \
        Z80_BYTE  Reg = (Register);                                     \
        Z80_WORD  Result = Z80_WORD(R.AF.B.h) - Z80_WORD(Reg);          \
        SET_OVERFLOW_FLAG_A_SUB(Reg, Result);                           \
        SET_CARRY_FLAG_SUB(Result);                                     \
        SET_HALFCARRY(Reg, Result);                                     \
        R.AF.B.l = (R.AF.B.l & (Z80_HALFCARRY_FLAG | Z80_OVERFLOW_FLAG  \
                                | Z80_CARRY_FLAG))                      \
                   | Z80_SUBTRACT_FLAG | t.zeroSignTable[Result & 0xFF] \
                   | (Reg & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2));     \
}

#define CP_A_R(Register)        CP_A_X(Register)

#define SBC_A_X(Register)                                               \
{                                                                       \
        Z80_BYTE  Reg = (Register);                                     \
        Z80_WORD  Result = Z80_WORD(R.AF.B.h) - Z80_WORD(Reg)           \
                           - Z80_WORD(Z80_FLAGS_REG & Z80_CARRY_FLAG);  \
        SET_OVERFLOW_FLAG_A_SUB(Reg, Result);                           \
        SET_CARRY_FLAG_SUB(Result);                                     \
        SET_HALFCARRY(Reg, Result);                                     \
        R.AF.B.h = Z80_BYTE(Result);                                    \
        R.AF.B.l = (R.AF.B.l & (Z80_HALFCARRY_FLAG | Z80_OVERFLOW_FLAG  \
                                | Z80_CARRY_FLAG))                      \
                   | Z80_SUBTRACT_FLAG | t.zeroSignTable2[R.AF.B.h];    \
}

#define SBC_A_R(Register)       SBC_A_X(Register)

#define ADD_RR_rr(Register1, Register2)                                 \
{                                                                       \
        Z80_LONG  Result = Z80_LONG(Register1) + Z80_LONG(Register2);   \
        SET_CARRY_FLAG_ADD16(Result);                                   \
        SET_HALFCARRY_16(Register1, Register2, Result);                 \
        Register1 = Z80_WORD(Result);                                   \
        Z80_FLAGS_REG = (Z80_FLAGS_REG & (0xD7 ^ Z80_SUBTRACT_FLAG))    \
                        | (Z80_BYTE(Result >> 8) & 0x28);               \
        updateCycles(7);                                                \
}

#define ADC_HL_rr(Register)                                             \
{                                                                       \
        Z80_LONG  Result = Z80_LONG(R.HL.W) + Z80_LONG(Register)        \
                           + Z80_LONG(Z80_FLAGS_REG & Z80_CARRY_FLAG);  \
        SET_OVERFLOW_FLAG_HL_ADD(Register, Result);                     \
        SET_CARRY_FLAG_ADD16(Result);                                   \
        SET_HALFCARRY_16(R.HL.W, Register, Result);                     \
        R.HL.W = Z80_WORD(Result);                                      \
        SET_ZERO_FLAG16(R.HL.W);                                        \
        Z80_FLAGS_REG = (Z80_FLAGS_REG & 0x55)                          \
                        | (Z80_BYTE(Result >> 8) & 0xA8);               \
        updateCycles(7);                                                \
}

#define SBC_HL_rr(Register)                                             \
{                                                                       \
        Z80_LONG  Result = Z80_LONG(R.HL.W) - Z80_LONG(Register)        \
                           - Z80_LONG(Z80_FLAGS_REG & Z80_CARRY_FLAG);  \
        SET_OVERFLOW_FLAG_HL_SUB(Register, Result);                     \
        SET_CARRY_FLAG_SUB16(Result);                                   \
        SET_HALFCARRY_16(R.HL.W, Register, Result);                     \
        R.HL.W = Z80_WORD(Result);                                      \
        SET_ZERO_FLAG16(R.HL.W);                                        \
        Z80_FLAGS_REG = (Z80_FLAGS_REG & 0x55) | Z80_SUBTRACT_FLAG      \
                        | (Z80_BYTE(Result >> 8) & 0xA8);               \
        updateCycles(7);                                                \
}

/* do a SLA of index and copy into reg specified */
#define INDEX_CB_SLA_REG(Reg)                                           \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SLA_WITH_FLAGS(tempByte);                                       \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/* do a SRA of index and copy into reg specified */
#define INDEX_CB_SRA_REG(Reg)                                           \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SRA_WITH_FLAGS(tempByte);                                       \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/* do a SLL of index and copy into reg specified */
#define INDEX_CB_SLL_REG(Reg)                                           \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SLL_WITH_FLAGS(tempByte);                                       \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

/* do a SRL of index and copy into reg specified */
#define INDEX_CB_SRL_REG(Reg)                                           \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SRL_WITH_FLAGS(tempByte);                                       \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define INDEX_CB_RLC_REG(Reg)                                           \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RLC_WITH_FLAGS(tempByte);                                       \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define INDEX_CB_RRC_REG(Reg)                                           \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RRC_WITH_FLAGS(tempByte);                                       \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define INDEX_CB_RR_REG(Reg)                                            \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RR_WITH_FLAGS(tempByte);                                        \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define INDEX_CB_RL_REG(Reg)                                            \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RL_WITH_FLAGS(tempByte);                                        \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define INDEX_CB_SET_REG(OrMask, Reg)                                   \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        SET(OrMask, tempByte);                                          \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define INDEX_CB_RES_REG(AndMask, Reg)                                  \
{                                                                       \
        Z80_BYTE  tempByte = RD_BYTE_INDEX();                           \
        RES(AndMask, tempByte);                                         \
        Reg = tempByte;                                                 \
        updateCycle();                                                  \
        WR_BYTE_INDEX(tempByte);                                        \
}

#define PrefixIgnore()                                                  \
        R.Flags &= ~Z80_CHECK_INTERRUPT_FLAG;                           \
        R.PC.W.l++;                                                     \
        INC_REFRESH(1)

/*---------------------------*/
/* pop a word from the stack */

/*-------------------------*/
/* put a word on the stack */

#define PUSH(Data)              pushWord(Data)

/*----------------------------------------------------*/
/* perform a RST which is equivalent to a 1 byte CALL */

#define RST(Addr)                                                       \
{                                                                       \
        /* push return address on stack */                              \
        PUSH(Z80_WORD(R.PC.W.l + 1));                                   \
        /* set program counter to address */                            \
        R.PC.W.l = Addr;                                                \
}

#define LD_R_R(Reg1, Reg2)      (Reg1 = (Reg2))

#define SET_IM(x)               (R.IM = (x))

#define HALT()                  (R.Flags |= Z80_EXECUTING_HALT_FLAG)

#define LD_I_A()                (R.I = R.AF.B.h)

/* increment register pair */
#define INC_rp(x)                                                       \
{                                                                       \
        x++;                                                            \
        updateCycles(2);                                                \
}

/* decrement register pair */
#define DEC_rp(x)                                                       \
{                                                                       \
        x--;                                                            \
        updateCycles(2);                                                \
}

/* swap two words */
#define SWAP(Reg1, Reg2)                                                \
{                                                                       \
        Z80_WORD  tempR = Reg1;                                         \
        Reg1 = Reg2;                                                    \
        Reg2 = tempR;                                                   \
}

#define JP_rp(x)                (R.PC.W.l = (x))

#define LD_SP_rp(x)                                                     \
{                                                                       \
        R.SP.W = (x);                                                   \
        updateCycles(2);                                                \
}

/* EX (SP), HL */
#define EX_SP_rr(Register)                                              \
{                                                                       \
        Z80_WORD  temp = POP();                                         \
        PUSH(Register);                                                 \
        Register = temp;                                                \
        updateCycles(2);                                                \
}

#define ADD_PC(x)               (R.PC.W.l += (x))

#define RETN()                                                          \
{                                                                       \
        R.IFF1 = R.IFF2;                                                \
        R.PC.W.l = POP();                                               \
}

#define RETI()                                                          \
{                                                                       \
        R.IFF1 = R.IFF2;                                                \
        R.PC.W.l = POP();                                               \
}

#define NEG()                                                           \
{                                                                       \
        Z80_BYTE    Flags;                                              \
        Z80_BYTE    AReg;                                               \
        AReg = R.AF.B.h;                                                \
        Flags = Z80_SUBTRACT_FLAG;                                      \
        if (AReg == 0x080)                                              \
        {                                                               \
            Flags |= Z80_PARITY_FLAG;                                   \
        }                                                               \
        if (AReg != 0x000)                                              \
        {                                                               \
            Flags |= Z80_CARRY_FLAG;                                    \
        }                                                               \
        if ((AReg & 0x0f)!=0)                                           \
        {                                                               \
            Flags |= Z80_HALFCARRY_FLAG;                                \
        }                                                               \
        R.AF.B.h = -AReg;                                               \
        Flags |= t.zeroSignTable[R.AF.B.h];                             \
        Flags |= R.AF.B.h & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);      \
        Z80_FLAGS_REG = Flags;                                          \
}

#define ADD_A_INDEX(Index)      ADD_A_X(RD_BYTE_INDEX_(Index))

#define ADC_A_INDEX(Index)      ADC_A_X(RD_BYTE_INDEX_(Index))

#define SUB_A_INDEX(Index)      SUB_A_X(RD_BYTE_INDEX_(Index))

#define SBC_A_INDEX(Index)      SBC_A_X(RD_BYTE_INDEX_(Index))

#define CP_A_INDEX(Index)       CP_A_X(RD_BYTE_INDEX_(Index))

#define RLA()                                                           \
{                                                                       \
        Z80_BYTE    Flags = Z80_FLAGS_REG;                              \
        Z80_BYTE    OrByte = Flags & 0x01;                              \
        Flags = Flags & (Z80_SIGN_FLAG | Z80_ZERO_FLAG | Z80_PARITY_FLAG); \
        Flags = Flags | ((R.AF.B.h >> 7) & 0x01);                       \
        Z80_BYTE    Reg = (R.AF.B.h << 1) | OrByte;                     \
        R.AF.B.h = Reg;                                                 \
        Flags = Flags | (Reg & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2));  \
        Z80_FLAGS_REG = Flags;                                          \
}

#define RLCA()                                                          \
{                                                                       \
        Z80_BYTE        OrByte;                                         \
        Z80_BYTE        Flags;                                          \
        OrByte = (R.AF.B.h>>7) & 0x01;                                  \
        Flags = Z80_FLAGS_REG;                                          \
        Flags = Flags & (Z80_SIGN_FLAG | Z80_ZERO_FLAG | Z80_PARITY_FLAG); \
        Flags |= OrByte;                                                \
        Z80_BYTE  Reg;                                                  \
        Reg = R.AF.B.h;                                                 \
        Reg = Reg<<1;                                                   \
        Reg = Reg|OrByte;                                               \
        R.AF.B.h=Reg;                                                   \
        Reg &= (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);                   \
        Flags = Flags | Reg;                                            \
        Z80_FLAGS_REG = Flags;                                          \
}

#define RRCA()                                                          \
{                                                                       \
        Z80_BYTE        Flags;                                          \
        Z80_BYTE        OrByte;                                         \
        OrByte = R.AF.B.h & 0x01;                                       \
        Flags = Z80_FLAGS_REG;                                          \
        Flags = Flags & (Z80_SIGN_FLAG | Z80_ZERO_FLAG | Z80_PARITY_FLAG); \
        Flags |= OrByte;                                                \
        OrByte = OrByte<<7;                                             \
        Z80_BYTE  Reg;                                                  \
        Reg = R.AF.B.h;                                                 \
        Reg = Reg>>1;                                                   \
        Reg = Reg | OrByte;                                             \
        R.AF.B.h = Reg;                                                 \
        Reg = Reg & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);              \
        Flags = Flags | Reg;                                            \
        Z80_FLAGS_REG = Flags;                                          \
}

/*-----*/
/* CPL */

#define CPL()                                                           \
{                                                                       \
        Z80_BYTE  Flags;                                                \
        /* complement */                                                \
        R.AF.B.h = (Z80_BYTE)(~R.AF.B.h);                               \
        Flags = Z80_FLAGS_REG;                                          \
        Flags = Flags & (Z80_SIGN_FLAG | Z80_ZERO_FLAG | Z80_PARITY_FLAG | Z80_CARRY_FLAG); \
        Flags |= R.AF.B.h & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);      \
        Flags |= Z80_SUBTRACT_FLAG | Z80_HALFCARRY_FLAG;                \
        Z80_FLAGS_REG = Flags;                                          \
}

/*-----*/
/* SCF */
#define SCF()                                                           \
{                                                                       \
        Z80_BYTE  Flags;                                                \
        Flags = Z80_FLAGS_REG;                                          \
        Flags = Flags & (Z80_ZERO_FLAG | Z80_PARITY_FLAG | Z80_SIGN_FLAG); \
        Flags = Flags | Z80_CARRY_FLAG;                                 \
        Flags |= R.AF.B.h & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);      \
        Z80_FLAGS_REG = Flags;                                          \
}

/*-----*/
/* CCF */
#define CCF()                                                           \
{                                                                       \
        Z80_BYTE  Flags;                                                \
        Flags = Z80_FLAGS_REG;                                          \
        Flags &= (Z80_CARRY_FLAG | Z80_ZERO_FLAG | Z80_PARITY_FLAG | Z80_SIGN_FLAG); \
        Flags |= ((Flags & Z80_CARRY_FLAG)<<Z80_HALFCARRY_FLAG_BIT);    \
        Flags |= R.AF.B.h & (Z80_UNUSED_FLAG1 | Z80_UNUSED_FLAG2);      \
        Z80_FLAGS_REG = Flags ^ Z80_CARRY_FLAG;                         \
}

/*--------*/
/* LD R,A */
#define LD_R_A()                                                        \
{                                                                       \
        Z80_BYTE        A;                                              \
        A = R.AF.B.h;                                                   \
        /* store bit 7 */                                               \
        R.RBit7 = A & 0x080;                                            \
        /* store refresh register */                                    \
        R.R = A & 0x07f;                                                \
}

/*--------*/
/* LD A,I */

#define LD_A_I()                                                        \
{                                                                       \
        R.AF.B.h = R.I;                                                 \
        Z80_BYTE        Flags;                                          \
        Flags = Z80_FLAGS_REG;                                          \
        Flags &= Z80_CARRY_FLAG;                                        \
        Flags |= ((R.IFF2 & 0x01)<<Z80_PARITY_FLAG_BIT);                \
        Flags |= t.zeroSignTable2[R.AF.B.h];                            \
        Z80_FLAGS_REG = Flags;                                          \
}

/*--------*/
/* LD A,R */
#define LD_A_R()                                                        \
{                                                                       \
        R.AF.B.h = R.RBit7 | (R.R & 0x07f);                             \
        Z80_BYTE        Flags;                                          \
        Flags = Z80_FLAGS_REG;                                          \
        Flags &= Z80_CARRY_FLAG;                                        \
        Flags |= ((R.IFF2 & 0x01)<<Z80_PARITY_FLAG_BIT);                \
        Flags |= t.zeroSignTable2[R.AF.B.h];                            \
        Z80_FLAGS_REG = Flags;                                          \
}

/* LDI */
#define LDI()                                                           \
{                                                                       \
        Z80_BYTE  Data = readMemory(R.HL.W);                            \
        writeMemory(R.DE.W, Data);                                      \
        R.HL.W++;                                                       \
        R.DE.W++;                                                       \
        R.BC.W--;                                                       \
        /* if BC == 0, then PV = 0, else PV = 1 */                      \
        Data = Data + R.AF.B.h;                                         \
        Z80_FLAGS_REG =                                                 \
            (Z80_FLAGS_REG & (Z80_CARRY_FLAG | Z80_ZERO_FLAG | Z80_SIGN_FLAG)) \
            | (Data & Z80_UNUSED_FLAG2) | ((Data & 0x02) << 4)          \
            | (R.BC.W == 0 ? 0x00 : Z80_PARITY_FLAG);                   \
}

/* LDD */
#define LDD()                                                           \
{                                                                       \
        Z80_BYTE  Data = readMemory(R.HL.W);                            \
        writeMemory(R.DE.W, Data);                                      \
        R.HL.W--;                                                       \
        R.DE.W--;                                                       \
        R.BC.W--;                                                       \
        /* if BC == 0, then PV = 0, else PV = 1 */                      \
        Data = Data + R.AF.B.h;                                         \
        Z80_FLAGS_REG =                                                 \
            (Z80_FLAGS_REG & (Z80_CARRY_FLAG | Z80_ZERO_FLAG | Z80_SIGN_FLAG)) \
            | (Data & Z80_UNUSED_FLAG2) | ((Data & 0x02) << 4)          \
            | (R.BC.W == 0 ? 0x00 : Z80_PARITY_FLAG);                   \
}

#define LD_HL_nnnn()                                                    \
{                                                                       \
        Z80_WORD  Addr;                                                 \
        Addr =  readOpcodeWord(1);                                      \
        R.HL.W = readMemoryWord(Addr);                                  \
}

#define LD_nnnn_HL()                                                    \
{                                                                       \
        Z80_WORD  Addr;                                                 \
        Addr =  readOpcodeWord(1);                                      \
        writeMemoryWord(Addr,R.HL.W);                                   \
}

#define LD_A_nnnn()                                                     \
{                                                                       \
        Z80_WORD  Addr;                                                 \
        Addr = readOpcodeWord(1);                                       \
        R.AF.B.h = readMemory(Addr);                                    \
}

#define LD_nnnn_A()                                                     \
{                                                                       \
        Z80_WORD  Addr;                                                 \
        Addr = readOpcodeWord(1);                                       \
        writeMemory(Addr,R.AF.B.h);                                     \
}

/*-----------------------------------*/
/* perform a return from sub-routine */
#define RETURN()                                                        \
{                                                                       \
        /* get return address from stack */                             \
        R.PC.W.l = POP();                                               \
}

#define DI()                                                            \
{                                                                       \
        R.IFF1 = R.IFF2 = 0;                                            \
        R.Flags &= ~Z80_CHECK_INTERRUPT_FLAG;                           \
}

#define EI()                                                            \
{                                                                       \
        R.IFF1 = R.IFF2 = 1;                                            \
        R.Flags &= ~Z80_CHECK_INTERRUPT_FLAG;                           \
}

