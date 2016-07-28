
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2010 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/ep128emu/
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

#include "ep128emu.hpp"
#include "gui.hpp"
#include "debuglib.hpp"
#include "debugger.hpp"

Ep128EmuGUI_LuaScript::~Ep128EmuGUI_LuaScript()
{
}

void Ep128EmuGUI_LuaScript::errorCallback(const char *msg)
{
  if (msg == (char *) 0 || msg[0] == '\0')
    msg = "Lua script error";
  debugWindow.gui.errorMessage(msg);
}

void Ep128EmuGUI_LuaScript::messageCallback(const char *msg)
{
  if (!msg)
    msg = "";
  debugWindow.monitor_->printMessage(msg);
}

Ep128EmuGUI_ScrollableOutput::~Ep128EmuGUI_ScrollableOutput()
{
}

int Ep128EmuGUI_ScrollableOutput::handle(int evt)
{
  if (evt == FL_MOUSEWHEEL) {
    int     tmp = Fl::event_dy();
    if (tmp > 0) {
      if (downWidget)
        downWidget->do_callback();
    }
    else if (tmp < 0) {
      if (upWidget)
        upWidget->do_callback();
    }
    return 1;
  }
  return Fl_Multiline_Output::handle(evt);
}

// ----------------------------------------------------------------------------

Ep128EmuGUI_DebugWindow::Ep128EmuGUI_DebugWindow(Ep128EmuGUI& gui_)
  : gui(gui_),
    luaScript(*this, gui_.vm)
{
  for (size_t i = 0; i < sizeof(windowTitle); i++)
    windowTitle[i] = '\0';
  std::strcpy(&(windowTitle[0]), "ep128emu debugger");
  savedWindowPositionX = 32;
  savedWindowPositionY = 32;
  focusWidget = (Fl_Widget *) 0;
  prvTab = (Fl_Widget *) 0;
  memoryDumpStartAddress = 0x00000000U;
  memoryDumpViewAddress = 0x00000000U;
  memoryDumpAddressMode = 1;
  ixViewOffset = 0;
  iyViewOffset = 0;
  disassemblyStartAddress = 0x00000000U;
  disassemblyViewAddress = 0x00000000U;
  disassemblyNextAddress = 0x00000000U;
  for (int i = 0; i < 6; i++)
    breakPointLists[i] = "";
  tmpBuffer.reserve(960);
  bpEditBuffer = new Fl_Text_Buffer();
  scriptEditBuffer = new Fl_Text_Buffer();
  createDebugWindow();
  window->label(&(windowTitle[0]));
  memoryDumpDisplay->upWidget = memoryDumpPrvPageButton;
  memoryDumpDisplay->downWidget = memoryDumpNxtPageButton;
  memoryDisplay_IX->upWidget = ixPrvPageButton;
  memoryDisplay_IX->downWidget = ixNxtPageButton;
  memoryDisplay_IY->upWidget = iyPrvPageButton;
  memoryDisplay_IY->downWidget = iyNxtPageButton;
  disassemblyDisplay->upWidget = disassemblyPrvPageButton;
  disassemblyDisplay->downWidget = disassemblyNxtPageButton;
}

Ep128EmuGUI_DebugWindow::~Ep128EmuGUI_DebugWindow()
{
  delete window;
  delete bpEditBuffer;
  delete scriptEditBuffer;
}

void Ep128EmuGUI_DebugWindow::show()
{
  this->activate();
  monitor_->closeTraceFile();
  updateWindow();
  if (!window->shown()) {
    window->resize(savedWindowPositionX, savedWindowPositionY, 960, 720);
    if (focusWidget != (Fl_Widget *) 0 && focusWidget != monitor_)
      focusWidget->take_focus();
    else
      stepButton->take_focus();
  }
  window->label(&(windowTitle[0]));
  window->show();
}

bool Ep128EmuGUI_DebugWindow::shown() const
{
  return bool(window->shown());
}

void Ep128EmuGUI_DebugWindow::hide()
{
  this->deactivate(1000000.0);
  if (window->shown()) {
    savedWindowPositionX = window->x();
    savedWindowPositionY = window->y();
  }
  window->hide();
  std::strcpy(&(windowTitle[0]), "ep128emu debugger");
}

void Ep128EmuGUI_DebugWindow::activate()
{
  Fl::remove_timeout(&hideWindowCallback, (void *) this);
  debugTabs->clear_output();
  mainTab->clear_output();
  monitorTab->clear_output();
  stepIntoButton->clear_output();
  returnButton->clear_output();
  stepToButton->clear_output();
  stepToAddressValuator->clear_output();
  stepOverButton->clear_output();
  stepButton->clear_output();
  continueButton->clear_output();
}

bool Ep128EmuGUI_DebugWindow::active() const
{
  if (!window->shown())
    return false;
  return (!continueButton->output());
}

void Ep128EmuGUI_DebugWindow::deactivate(double tt)
{
  Fl::remove_timeout(&hideWindowCallback, (void *) this);
  if (tt <= 0.0) {
    this->hide();
    return;
  }
  mainTab->set_output();
  monitorTab->set_output();
  debugTabs->set_output();
  stepIntoButton->set_output();
  returnButton->set_output();
  stepToButton->set_output();
  stepToAddressValuator->set_output();
  stepOverButton->set_output();
  stepButton->set_output();
  continueButton->set_output();
  if (gui.debugWindowOpenFlag) {
    gui.debugWindowOpenFlag = false;
    gui.unlockVMThread();
  }
  if (tt < 1000.0)
    Fl::add_timeout(tt, &hideWindowCallback, (void *) this);
}

bool Ep128EmuGUI_DebugWindow::breakPoint(int type, uint16_t addr, uint8_t value)
{
  if ((type == 0 || type == 3) && monitor_->getIsTraceOn()) {
    monitor_->writeTraceFile(addr);
    if (type == 3)
      return false;
  }
  switch (type) {
  case 0:
  case 3:
    try {
      gui.vm.disassembleInstruction(tmpBuffer, addr, true);
      if (tmpBuffer.length() > 21 && tmpBuffer.length() <= 40) {
        std::sprintf(&(windowTitle[0]), "Break at PC=%04X: %s",
                     (unsigned int) (addr & 0xFFFF), (tmpBuffer.c_str() + 21));
        tmpBuffer = "";
        break;
      }
    }
    catch (...) {
    }
    tmpBuffer = "";
  case 1:
    std::sprintf(&(windowTitle[0]),
                 "Break on reading %02X from memory address %04X",
                 (unsigned int) (value & 0xFF), (unsigned int) (addr & 0xFFFF));
    break;
  case 2:
    std::sprintf(&(windowTitle[0]),
                 "Break on writing %02X to memory address %04X",
                 (unsigned int) (value & 0xFF), (unsigned int) (addr & 0xFFFF));
    break;
  case 5:
    std::sprintf(&(windowTitle[0]),
                 "Break on reading %02X from I/O port %02X",
                 (unsigned int) (value & 0xFF), (unsigned int) (addr & 0xFF));
    break;
  case 6:
    std::sprintf(&(windowTitle[0]),
                 "Break on writing %02X to I/O port %02X",
                 (unsigned int) (value & 0xFF), (unsigned int) (addr & 0xFF));
    break;
  default:
    std::sprintf(&(windowTitle[0]), "Break");
  }
  disassemblyViewAddress = uint32_t(gui.vm.getProgramCounter() & 0xFFFF);
  if (focusWidget == monitor_)
    monitor_->breakMessage(&(windowTitle[0]));
  return true;
}

void Ep128EmuGUI_DebugWindow::updateWindow()
{
  try {
    gui.vm.listCPURegisters(tmpBuffer);
    cpuRegisterDisplay->value(tmpBuffer.c_str());
    {
      char  tmpBuf[64];
      std::sprintf(&(tmpBuf[0]), "0000-3FFF: %02X\n4000-7FFF: %02X\n"
                                 "8000-BFFF: %02X\nC000-FFFF: %02X",
                   (unsigned int) gui.vm.getMemoryPage(0),
                   (unsigned int) gui.vm.getMemoryPage(1),
                   (unsigned int) gui.vm.getMemoryPage(2),
                   (unsigned int) gui.vm.getMemoryPage(3));
      memoryPagingDisplay->value(&(tmpBuf[0]));
    }
    uint32_t  tmp = gui.vm.getStackPointer();
    uint32_t  startAddr = (tmp + 0xFFF4U) & 0xFFF8U;
    uint32_t  endAddr = (startAddr + 0x002FU) & 0xFFFFU;
    dumpMemory(tmpBuffer, startAddr, endAddr, tmp, true, 1);
    stackMemoryDumpDisplay->value(tmpBuffer.c_str());
    tmpBuffer = "";
    updateMemoryDumpDisplay();
    updateIOPortDisplay();
    updateDisassemblyDisplay();
    bpPriorityThresholdValuator->value(
        double(gui.config.debug.bpPriorityThreshold));
  }
  catch (std::exception& e) {
    gui.errorMessage(e.what());
  }
}

void Ep128EmuGUI_DebugWindow::dumpMemory(std::string& buf,
                                         uint32_t startAddr, uint32_t endAddr,
                                         uint32_t cursorAddr, bool showCursor,
                                         uint8_t addressMode)
{
  try {
    char      tmpBuf[48];
    uint8_t   tmpBuf2[8];
    buf.clear();
    uint32_t  addrMask = uint32_t(addressMode != 0 ? 0x0000FFFFU : 0x003FFFFFU);
    startAddr &= addrMask;
    endAddr &= addrMask;
    cursorAddr &= addrMask;
    if (addressMode > 1 && !showCursor) {
      showCursor = true;
      cursorAddr = 0xFFFFFFFFU;
    }
    while (true) {
      char    *bufp = &(tmpBuf[0]);
      Ep128Emu::printHexNumber(bufp, startAddr, (addressMode != 0 ? 2 : 0),
                               (addressMode != 0 ? 4 : 6),
                               (showCursor ? 0 : 1));
      bufp = bufp + (showCursor ? 6 : 7);
      int     cnt = 7;
      do {
        if (addressMode < 2)
          tmpBuf2[cnt] = gui.vm.readMemory(startAddr, bool(addressMode));
        else
          tmpBuf2[cnt] = gui.vm.readIOPort(uint16_t(startAddr));
        Ep128Emu::printHexNumber(bufp,
                                 tmpBuf2[cnt], (showCursor ? 2 : 1), 2, 0);
        if (showCursor && startAddr == cursorAddr)
          bufp[1] = '*';
        bufp = bufp + (showCursor ? 4 : 3);
        if (startAddr == endAddr)
          break;
        startAddr = (startAddr + 1U) & addrMask;
      } while (--cnt >= 0);
      if (cnt <= 0 && !showCursor) {
        bufp[0] = ' ';
        bufp[1] = ':';
        for (int i = 2; i < 10; i++) {
          bufp[i] = char(tmpBuf2[9 - i] & 0x7F);
          if (bufp[i] < char(0x20) || bufp[i] == char(0x7F))
            bufp[i] = '.';
        }
        bufp[10] = '\0';
        bufp = bufp + 10;
      }
      if (cnt >= 0)
        break;
      bufp[0] = '\n';
      bufp[1] = '\0';
      buf += &(tmpBuf[0]);
    }
    buf += &(tmpBuf[0]);
  }
  catch (std::exception& e) {
    buf.clear();
    gui.errorMessage(e.what());
  }
}

char * Ep128EmuGUI_DebugWindow::dumpMemoryAtRegister(
    char *bufp, const char *regName, uint16_t addr, int byteCnt,
    uint32_t cursorAddr)
{
  while ((*regName) != '\0')
    *(bufp++) = *(regName++);
  while (byteCnt-- > 0) {
    char    *nxtp =
        Ep128Emu::printHexNumber(bufp, gui.vm.readMemory(addr, true), 2, 2, 0);
    if (uint32_t(addr) == cursorAddr)
      bufp[1] = '*';
    bufp = nxtp;
    addr = (addr + 1) & 0xFFFF;
  }
  *bufp = '\0';
  return bufp;
}

void Ep128EmuGUI_DebugWindow::updateMemoryDumpDisplay()
{
  try {
    uint32_t  addrMask =
        uint32_t(memoryDumpAddressMode != 0 ? 0x00FFFFU : 0x3FFFFFU);
    memoryDumpStartAddress &= addrMask;
    memoryDumpViewAddress &= addrMask;
    const char  *fmt = (memoryDumpAddressMode != 0 ? "%04X" : "%06X");
    char  tmpBuf[64];
    std::sprintf(&(tmpBuf[0]), fmt, (unsigned int) memoryDumpStartAddress);
    memoryDumpStartAddressValuator->value(&(tmpBuf[0]));
    dumpMemory(tmpBuffer, memoryDumpViewAddress, memoryDumpViewAddress + 0x2FU,
               0U, false, memoryDumpAddressMode);
    memoryDumpDisplay->value(tmpBuffer.c_str());
    const Ep128::Z80_REGISTERS& r =
        ((const Ep128Emu::VirtualMachine *) &(gui.vm))->getZ80Registers();
    {
      char    *bufp = dumpMemoryAtRegister(&(tmpBuf[0]), " BC-04",
                                           (r.BC.W - 4) & 0xFFFF, 12, r.BC.W);
      *(bufp++) = '\n';
      *bufp = '\0';
      tmpBuffer = &(tmpBuf[0]);
      bufp = dumpMemoryAtRegister(&(tmpBuf[0]), " DE-04",
                                  (r.DE.W - 4) & 0xFFFF, 12, r.DE.W);
      *(bufp++) = '\n';
      *bufp = '\0';
      tmpBuffer += &(tmpBuf[0]);
      bufp = dumpMemoryAtRegister(&(tmpBuf[0]), " HL-04",
                                  (r.HL.W - 4) & 0xFFFF, 12, r.HL.W);
      tmpBuffer += &(tmpBuf[0]);
    }
    memoryDisplay_BC_DE_HL->value(tmpBuffer.c_str());
    for (int i = 0; i < 6; i++) {
      char      *bufp = &(tmpBuf[0]);
      uint16_t  addr = uint16_t(i < 3 ? r.IX.W : r.IY.W);
      int32_t   offs = (i < 3 ? ixViewOffset : (iyViewOffset - 24))
                       + int32_t((i - 1) * 8);
      *(bufp++) = ' ';
      *(bufp++) = 'I';
      *(bufp++) = (i < 3 ? 'X' : 'Y');
      *(bufp++) = (offs >= 0 ? '+' : '-');
      bufp = Ep128Emu::printHexNumber(bufp, uint32_t(offs >= 0 ?
                                                     offs : (-offs)) & 0xFFU,
                                      0, 2, 0);
      bufp = dumpMemoryAtRegister(bufp, "",
                                  uint16_t((int32_t(addr) + offs) & 0xFFFF), 8);
      if (i != 2 && i != 5) {
        *(bufp++) = '\n';
        *bufp = '\0';
      }
      if (i == 0 || i == 3)
        tmpBuffer = &(tmpBuf[0]);
      else
        tmpBuffer += &(tmpBuf[0]);
      if (i == 2)
        memoryDisplay_IX->value(tmpBuffer.c_str());
      else if (i == 5)
        memoryDisplay_IY->value(tmpBuffer.c_str());
    }
    tmpBuffer = "";
  }
  catch (std::exception& e) {
    gui.errorMessage(e.what());
  }
}

void Ep128EmuGUI_DebugWindow::updateIOPortDisplay()
{
  try {
    gui.vm.listIORegisters(tmpBuffer);
    ioPortDisplay->value(tmpBuffer.c_str());
    tmpBuffer = "";
  }
  catch (std::exception& e) {
    gui.errorMessage(e.what());
  }
}

void Ep128EmuGUI_DebugWindow::updateDisassemblyDisplay()
{
  try {
    disassemblyStartAddress &= 0xFFFFU;
    disassemblyViewAddress &= 0xFFFFU;
    disassemblyNextAddress &= 0xFFFFU;
    char  tmpBuf[8];
    std::sprintf(&(tmpBuf[0]), "%04X", (unsigned int) disassemblyStartAddress);
    disassemblyStartAddressValuator->value(&(tmpBuf[0]));
    std::string tmp;
    tmp.reserve(48);
    tmpBuffer = "";
    uint32_t  addr = disassemblySearchBack(2);
    uint32_t  pcAddr = uint32_t(gui.vm.getProgramCounter()) & 0xFFFFU;
    for (int i = 0; i < 23; i++) {
      if (i == 22)
        disassemblyNextAddress = addr;
      uint32_t  nxtAddr = gui.vm.disassembleInstruction(tmp, addr, true, 0);
      while (addr != nxtAddr) {
        if (addr == pcAddr)
          tmp[1] = '*';
        addr = (addr + 1U) & 0xFFFFU;
      }
      tmpBuffer += tmp;
      if (i != 22)
        tmpBuffer += '\n';
    }
    disassemblyDisplay->value(tmpBuffer.c_str());
    tmpBuffer = "";
  }
  catch (std::exception& e) {
    gui.errorMessage(e.what());
  }
}

uint32_t Ep128EmuGUI_DebugWindow::disassemblySearchBack(int insnCnt)
{
  uint32_t  addrTable[256];
  uint8_t   insnLengths[256];
  if (insnCnt > 50)
    insnCnt = 50;
  for (uint32_t addr = disassemblyViewAddress - (uint32_t(insnCnt * 4) + 28U);
       true;
       addr++) {
    addr = addr & 0xFFFFU;
    uint32_t  tmp =
        Ep128::Z80Disassembler::getNextInstructionAddr(gui.vm, addr, true);
    insnLengths[addr & 0xFFU] = uint8_t((tmp - addr) & 0xFFU);
    if (addr == (disassemblyViewAddress & 0xFFFFU))
      break;
  }
  for (uint32_t offs = 28U; true; offs--) {
    int       addrCnt = 0;
    uint32_t  addr = disassemblyViewAddress - (uint32_t(insnCnt * 4) + offs);
    bool      doneFlag = false;
    addr = addr & 0xFFFFU;
    addrTable[addrCnt++] = addr;
    do {
      uint32_t  nxtAddr =
          (addr + uint32_t(insnLengths[addr & 0xFFU])) & 0xFFFFU;
      while (addr != nxtAddr) {
        if (addr == disassemblyViewAddress)
          doneFlag = true;
        addr = (addr + 1U) & 0xFFFFU;
      }
      addrTable[addrCnt++] = addr;
    } while (!doneFlag);
    for (int i = 0; i < addrCnt; i++) {
      if (addrTable[i] == disassemblyViewAddress) {
        if (i >= insnCnt)
          return addrTable[i - insnCnt];
      }
    }
    if (offs == 0U) {
      if (insnCnt < 2)
        break;
      if (insnCnt > 2)
        return addrTable[(addrCnt > insnCnt ? ((addrCnt - 1) - insnCnt) : 0)];
      insnCnt--;
      offs = 12U;
    }
  }
  return disassemblyViewAddress;
}

void Ep128EmuGUI_DebugWindow::applyBreakPointList()
{
  const char  *buf = (char *) 0;
  try {
    buf = bpEditBuffer->text();
    if (!buf)
      throw std::bad_alloc();
    std::string bpListText(buf);
    std::free(const_cast<char *>(buf));
    buf = (char *) 0;
    Ep128Emu::BreakPointList  bpList(bpListText);
    gui.vm.clearBreakPoints();
    gui.vm.setBreakPoints(bpList);
  }
  catch (std::exception& e) {
    if (buf)
      std::free(const_cast<char *>(buf));
    gui.errorMessage(e.what());
  }
}

void Ep128EmuGUI_DebugWindow::breakPointCallback(void *userData, int type,
                                                 uint16_t addr, uint8_t value)
{
  Ep128EmuGUI_DebugWindow&  debugWindow =
      *(reinterpret_cast<Ep128EmuGUI_DebugWindow *>(userData));
  if (!debugWindow.luaScript.runBreakPointCallback(type, addr, value)) {
    return;
  }
  Ep128EmuGUI&  gui_ = debugWindow.gui;
  Fl::lock();
  if (gui_.exitFlag || !gui_.mainWindow->shown()) {
    Fl::unlock();
    return;
  }
  if (!debugWindow.breakPoint(type, addr, value)) {
    Fl::unlock();
    return;             // do not show debugger window if tracing
  }
  if (!debugWindow.active()) {
    gui_.debugWindowShowFlag = true;
    Fl::awake();
  }
  while (gui_.debugWindowShowFlag) {
    Fl::unlock();
    gui_.updateDisplay();
    Fl::lock();
  }
  while (true) {
    bool  tmp = debugWindow.active();
    Fl::unlock();
    if (!tmp)
      break;
    gui_.updateDisplay();
    Fl::lock();
  }
}

void Ep128EmuGUI_DebugWindow::hideWindowCallback(void *userData)
{
  reinterpret_cast< Ep128EmuGUI_DebugWindow * >(userData)->hide();
}

