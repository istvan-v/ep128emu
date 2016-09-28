
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef EP128EMU_DEBUGLIB_HPP
#define EP128EMU_DEBUGLIB_HPP

#include "ep128emu.hpp"
#include "vm.hpp"

#include <vector>

namespace Ep128 {

  class Z80;

  class Z80Disassembler {
   private:
    static const char *opcodeNames;
    static const char *operandTypes[75];
    static const unsigned char opcodeTable[768];
    static const unsigned char opcodeTableCB[768];
    static const unsigned char opcodeTableED[768];
    static const unsigned char alternateOperandTypeTable[75];
    static const unsigned char noPrefixOperandTypeTable[75];
    static void parseOperand(const std::vector< std::string >& args,
                             size_t argOffs, size_t argCnt, int& opType,
                             bool& haveOpValue, uint32_t& opValue);
   public:
    /*!
     * Disassemble one Z80 instruction, reading from memory of virtual
     * machine 'vm', starting at address 'addr', and write the result to
     * 'buf' (not including a newline character). 'offs' is added to the
     * instruction address that is printed. The maximum line width is 40
     * characters.
     * Returns the address of the next instruction. If 'isCPUAddress' is
     * true, 'addr' is interpreted as a 16-bit CPU address, otherwise it
     * is assumed to be a 22-bit physical address (8 bit segment + 14 bit
     * offset).
     */
    static uint32_t disassembleInstruction(std::string& buf,
                                           const Ep128Emu::VirtualMachine& vm,
                                           uint32_t addr,
                                           bool isCPUAddress = false,
                                           int32_t offs = 0);
    // Same as disassembleInstruction() without actually writing to a string.
    static uint32_t getNextInstructionAddr(const Ep128Emu::VirtualMachine& vm,
                                           uint32_t addr,
                                           bool isCPUAddress = false);
    /*!
     * Assemble one Z80 instruction from 'args', which is expected to be
     * initialized with Ep128Emu::tokenizeString(), writing to the memory of
     * virtual machine 'vm'. 'offs' is added to the instruction address.
     * Returns the address of the instruction assembled.
     * If there is a syntax error, Ep128Emu::Exception is thrown.
     */
    static uint32_t assembleInstruction(const std::vector< std::string >& args,
                                        Ep128Emu::VirtualMachine& vm,
                                        bool isCPUAddress = false,
                                        int32_t offs = 0);
  };

  void listZ80Registers(std::string& buf, const Z80& z80);

}       // namespace Ep128

namespace Ep128Emu {

  void tokenizeString(std::vector<std::string>& args, const char *s);
  bool parseHexNumber(uint32_t& n, const char *s);
  uint32_t parseHexNumberEx(const char *s, uint32_t mask_ = 0xFFFFFFFFU);
  char * printHexNumber(char *bufp, uint32_t n,
                        size_t spaceCnt1, size_t nDigits, size_t spaceCnt2);

}       // namespace Ep128Emu

#endif  // EP128EMU_DEBUGLIB_HPP

