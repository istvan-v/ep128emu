
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

#include "ep128emu.hpp"
#include "ide.hpp"

namespace Ep128 {

  extern uint32_t checkVHDImage(std::FILE *imageFile, const char *fileName,
                                uint16_t& c, uint16_t& h, uint16_t& s)
  {
    c = 0;
    h = 0;
    s = 0;
    if (!imageFile) {
      throw Ep128Emu::Exception("checkVHDImage(): internal error: "
                                "imageFile == NULL");
    }
    bool    vhdExtension = false;
    if (fileName && fileName[0]) {
      size_t  nameLen = std::strlen(fileName);
      if (nameLen >= 4) {
        if (fileName[nameLen - 4] == '.' &&
            (fileName[nameLen - 3] | char(0x20)) == 'v' &&
            (fileName[nameLen - 2] | char(0x20)) == 'h' &&
            (fileName[nameLen - 1] | char(0x20)) == 'd') {
          vhdExtension = true;
        }
      }
    }
    if (std::fseek(imageFile, 0L, SEEK_END) < 0)
      throw Ep128Emu::Exception("error seeking IDE disk image");
    size_t  fileSize = size_t(std::ftell(imageFile));
    if (std::fseek(imageFile, 0L, SEEK_SET) < 0)
      throw Ep128Emu::Exception("error seeking IDE disk image");
    if (!(fileSize >= 0x000A0000 && fileSize <= 0x7FFFFE00))
      throw Ep128Emu::Exception("IDE disk image size is out of range");
    if ((fileSize & 0x01FF) != 0 && ((fileSize + 1) & 0x01FF) != 0) {
      throw Ep128Emu::Exception("invalid IDE disk image size "
                                "- must be an integer multiple of 512");
    }
    uint32_t  nSectors = uint32_t((fileSize + 511UL) >> 9);
    if (!(fileSize & 0x01FF) && !vhdExtension) {
      s = 17;
      uint32_t  ch = nSectors / s;
      h = uint16_t((ch + 1023U) >> 10);
      if (h < 4)
        h = 4;
      if (ch >= (uint32_t(h) << 10) || h > 16) {
        h = 16;
        s = 31;
        ch = nSectors / s;
      }
      if (ch >= (uint32_t(h) << 10)) {
        h = 16;
        s = 63;
        ch = nSectors / s;
      }
      c = uint16_t(ch / h);
    }
    if (vhdExtension || (fileSize & 0x03FF) != 0) {
      uint8_t buf[512];
      // check if the image file is in VHD format
      if (std::fseek(imageFile, long((nSectors - 1U) << 9), SEEK_SET) < 0)
        throw Ep128Emu::Exception("error seeking IDE disk image");
      if (std::fread(buf, sizeof(uint8_t), 511, imageFile) != 511)
        throw Ep128Emu::Exception("error reading IDE disk image");
      if (std::fseek(imageFile, 0L, SEEK_SET) < 0)
        throw Ep128Emu::Exception("error seeking IDE disk image");
      do {
        // check cookie (needed ?)
        if (!(buf[0] == 0x63 && buf[1] == 0x6F && buf[2] == 0x6E && // "con"
              buf[3] == 0x65 && buf[4] == 0x63 && buf[5] == 0x74 && // "ect"
              buf[6] == 0x69 && buf[7] == 0x78)) {                  // "ix"
          break;
        }
        nSectors--;
        // check features (must be less than or equal to 0x00000003)
        if ((buf[8] | buf[9] | buf[10] | (buf[11] & 0xFC)) != 0)
          break;
        // check format version (must be 0x00010000)
        if ((buf[12] | (buf[13] ^ 0x01) | buf[14] | buf[15]) != 0)
          break;
        // check data offset (must be 0xFFFFFFFF, i.e. fixed disk)
        if ((buf[16] & buf[17] & buf[18] & buf[19]) != 0xFF)
          break;
        // check size (must be less than 2 GB)
        if ((buf[48] | buf[49] | buf[50] | buf[51] | (buf[52] & 0x80)) != 0)
          break;
        // check if size matches the actual file size
        uint32_t  lbaSize =
            (uint32_t(buf[52]) << 24) | (uint32_t(buf[53]) << 16)
            | (uint32_t(buf[54]) << 8) | uint32_t(buf[55]);
        if ((lbaSize & 511U) || lbaSize > (nSectors << 9))
          break;
        lbaSize = lbaSize >> 9;
        // check disk geometry
        c = (uint16_t(buf[56]) << 8) | uint16_t(buf[57]);
        h = uint16_t(buf[58]);
        s = uint16_t(buf[59]);
        if (h > 16)
          break;
        uint32_t  tmp = uint32_t(c) * uint32_t(h) * uint32_t(s);
        if (!(tmp > 0U && tmp <= lbaSize))
          break;
        if (tmp < nSectors) {
          if (((tmp + (uint32_t(h) * uint32_t(s))) <= nSectors) ||
              ((tmp + (uint32_t(c) * uint32_t(s))) <= nSectors) ||
              ((tmp + (uint32_t(c) * uint32_t(h))) <= nSectors)) {
            break;
          }
        }
        nSectors = lbaSize;
        // check disk type (must be 0x00000002, i.e. fixed hard disk)
        if ((buf[60] | buf[61] | buf[62] | (buf[63] ^ 0x02)) != 0)
          break;
        // verify checksum
        tmp = (uint32_t(buf[64]) << 24) | (uint32_t(buf[65]) << 16)
              | (uint32_t(buf[66]) << 8) | uint32_t(buf[67]);
        for (int i = 0; i < 511; i++) {
          if (i == 64)
            i = 68;
          tmp = tmp + uint32_t(buf[i]);
        }
        if (tmp == 0xFFFFFFFFU)
          return nSectors;
      } while (false);
    }
    c = 0;
    h = 0;
    s = 0;
    if (vhdExtension)
      throw Ep128Emu::Exception("invalid or unsupported VHD file footer");
    if (fileSize & 0x01FF) {
      throw Ep128Emu::Exception("invalid IDE disk image size "
                                "- must be an integer multiple of 512");
    }
    throw Ep128Emu::Exception("invalid IDE disk image size "
                              "- cannot calculate geometry");
    return nSectors;
  }

  // --------------------------------------------------------------------------

  bool IDEInterface::IDEController::IDEDrive::convertCHSToLBA(
      uint32_t& b, uint16_t c, uint16_t h, uint16_t s)
  {
    b = nSectors;
    if (c >= nCylinders || h >= nHeads || s < 1 || s > nSectorsPerTrack)
      return false;
    b = (uint32_t(c) * uint32_t(nHeads) + uint32_t(h))
        * uint32_t(nSectorsPerTrack)
        + uint32_t(s - 1);
    return true;
  }

  bool IDEInterface::IDEController::IDEDrive::convertLBAToCHS(
      uint16_t& c, uint16_t& h, uint16_t& s, uint32_t b)
  {
    c = nCylinders;
    h = 0;
    s = 1;
    if (b >= nSectors)
      return false;
    s = uint16_t((b % uint32_t(nSectorsPerTrack)) + 1U);
    b = b / uint32_t(nSectorsPerTrack);
    h = uint16_t(b % uint32_t(nHeads));
    b = b / uint32_t(nHeads);
    c = uint16_t(b);
    return true;
  }

  void IDEInterface::IDEController::IDEDrive::readBlock()
  {
    if (sectorCnt < 1 || ideController.errorRegister != 0x00) {
      // command is complete, or there was an error in the previous block
      if (sectorCnt > 0)
        ideController.errorRegister |= uint8_t(0x04);   // command aborted
      commandDone(ideController.errorRegister);
      return;
    }
    // read sector(s) from disk: set BUSY flag, clear DRQ
    ideController.statusRegister = (ideController.statusRegister & 0x77) | 0x80;
    ledStateCounter = uint8_t(haveImageFile() ? 50 : 0);    // 100 ms
    size_t  blockSize = (ideController.commandRegister == 0xC4 ?
                         size_t(multSectCnt) : size_t(1));
    blockSize = (blockSize < size_t(sectorCnt) ? blockSize : size_t(sectorCnt));
    readWordCnt = uint16_t(blockSize << 8);
    bufPos = 0;
    size_t  bytesRead = 0;
    if ((currentSector + uint32_t(blockSize)) > nSectors) {
      ideController.errorRegister |= uint8_t(0x10);     // IDNF
      size_t  tmp = size_t(nSectors - currentSector);
      std::memset(buf + (tmp << 9), 0x00, (blockSize - tmp) << 9);
      blockSize = tmp;
    }
    if (blockSize > 0) {
      if (std::fseek(imageFile, long(currentSector << 9), SEEK_SET) < 0) {
        // error seeking disk image
        ideController.errorRegister |= uint8_t(0x10);
      }
      else {
        bytesRead = std::fread(buf, sizeof(uint8_t), blockSize << 9, imageFile);
        if (bytesRead < (blockSize << 9)) {
          // read error
          ideController.errorRegister |= uint8_t(0x40);
        }
      }
    }
    if (bytesRead < (blockSize << 9))
      std::memset(buf + bytesRead, 0x00, (blockSize << 9) - bytesRead);
    currentSector += uint32_t(bytesRead >> 9);
    sectorCnt -= uint16_t(bytesRead >> 9);
    if (sectorCnt < 1)
      currentSector--;
    // buffer is full, set interrupt and DRQ flag
    ideController.statusRegister |= uint8_t(0x88);
    interruptFlag = true;
  }

  void IDEInterface::IDEController::IDEDrive::writeBlockDone()
  {
    // write sector(s) to disk: set BUSY flag, clear DRQ
    ideController.statusRegister = (ideController.statusRegister & 0x77) | 0x80;
    ledStateCounter = uint8_t(haveImageFile() ? 50 : 0);    // 100 ms
    size_t  blockSize = (ideController.commandRegister == 0xC5 ?
                         size_t(multSectCnt) : size_t(1));
    blockSize = (blockSize < size_t(sectorCnt) ? blockSize : size_t(sectorCnt));
    writeWordCnt = uint16_t(blockSize << 8);
    bufPos = 0;
    size_t  bytesWritten = 0;
    if ((currentSector + uint32_t(blockSize)) > nSectors) {
      ideController.errorRegister |= uint8_t(0x10);     // IDNF
      blockSize = size_t(nSectors - currentSector);
    }
    if (blockSize > 0) {
      if (std::fseek(imageFile, long(currentSector << 9), SEEK_SET) < 0) {
        // error seeking disk image
        ideController.errorRegister |= uint8_t(0x10);
      }
      else {
        bytesWritten = std::fwrite(buf, sizeof(uint8_t), blockSize << 9,
                                   imageFile);
        if (bytesWritten < (blockSize << 9)) {
          // write error
          ideController.errorRegister |= uint8_t(0x40);
        }
        else if (ideController.commandRegister == 0x3C) {   // WRITE VERIFY
          if (std::fseek(imageFile, long(currentSector << 9), SEEK_SET) < 0) {
            // error seeking disk image
            ideController.errorRegister |= uint8_t(0x50);
          }
          else {
            uint8_t tmpBuf[512];
            for (size_t i = 0; i < blockSize; i++) {
              if (std::fread(&(tmpBuf[0]), sizeof(uint8_t), 512, imageFile)
                  != 512) {
                // read error
                ideController.errorRegister |= uint8_t(0x40);
                break;
              }
              if (std::memcmp(&(tmpBuf[0]), buf, 512) != 0) {
                // read error
                ideController.errorRegister |= uint8_t(0x40);
                break;
              }
            }
          }
        }
      }
    }
    currentSector += uint32_t(bytesWritten >> 9);
    sectorCnt -= uint16_t(bytesWritten >> 9);
    if (sectorCnt < 1 || ideController.errorRegister != 0x00) {
      // command is complete, or there was an error in the previous block
      if (sectorCnt > 0)
        ideController.errorRegister |= uint8_t(0x04);   // command aborted
      else
        currentSector--;
      commandDone(ideController.errorRegister);
      return;
    }
    // buffer is empty, set interrupt and DRQ flag
    ideController.statusRegister |= uint8_t(0x88);
    interruptFlag = true;
  }

  void IDEInterface::IDEController::IDEDrive::ackMediaChangeCommand()
  {
    if (!(ideController.statusRegister & 0x40)) {
      commandDone(0x04);        // drive not ready: abort
      return;
    }
    diskChangeFlag = false;
    commandDone(0x00);
  }

  void IDEInterface::IDEController::IDEDrive::getMediaStatusCommand()
  {
    if (!haveImageFile())
      ideController.errorRegister |= uint8_t(0x02);     // NM
    if (diskChangeFlag) {
      ideController.errorRegister |= uint8_t(0x20);     // MC
      diskChangeFlag = false;
    }
    if (readOnlyMode && haveImageFile())
      ideController.errorRegister |= uint8_t(0x40);     // WP
    commandDone(ideController.errorRegister);
  }

  void IDEInterface::IDEController::IDEDrive::identifyDeviceCommand()
  {
    if (!(ideController.statusRegister & 0x40)) {
      commandDone(0x04);        // drive not ready: abort
      return;
    }
    for (int i = 0; i < 512; i++)
      buf[i] = 0x00;
    if (defaultCylinders != 136)
      buf[0] = 0x88;            // removable device, not MFM
    else
      buf[0] = 0x80;            // hack to avoid error in IDE.ROM
    buf[2] = uint8_t(defaultCylinders & 0x00FF);
    buf[3] = uint8_t(defaultCylinders >> 8);
    buf[6] = uint8_t(defaultHeads & 0x00FF);
    buf[7] = uint8_t(defaultHeads >> 8);
    buf[9] = uint8_t((defaultSectorsPerTrack << 1) & 0xFF);
    buf[11] = 0x02;             // 512 bytes per sector
    buf[12] = uint8_t(defaultSectorsPerTrack & 0x00FF);
    buf[13] = uint8_t(defaultSectorsPerTrack >> 8);
    char    tmpBuf[64];
    char    *s = &(tmpBuf[0]);  // serial number
    std::sprintf(s, "            %08X",
                 (unsigned int) (uintptr_t((void *) imageFile) & 0xFFFFFFFFUL));
    for (int i = 20; i < 40; i++) {
      char    c = ' ';
      if ((*s) != '\0')
        c = *(s++);
      buf[i ^ 1] = uint8_t(c);
    }
    s = &(tmpBuf[0]);           // firmware revision
    std::sprintf(s, "EP_20A00");
    for (int i = 46; i < 54; i++) {
      char    c = ' ';
      if ((*s) != '\0')
        c = *(s++);
      buf[i ^ 1] = uint8_t(c);
    }
    s = &(tmpBuf[0]);           // model number
    {
      int     n =
          std::sprintf(s, "ep128emu IDE disk%s %u%s",
                       (vhdFormat ? " (VHD)" : ""),
                       (nSectors < 19999U ?
                        (unsigned int) ((nSectors + 1U) >> 1)
                        : (unsigned int) ((nSectors + 1024U) >> 11)),
                       (nSectors < 19999U ? "KB" : "MB"));
      if (!vhdFormat) {
        std::sprintf(s + n, " %u/%u/%u",
                     (unsigned int) defaultCylinders,
                     (unsigned int) defaultHeads,
                     (unsigned int) defaultSectorsPerTrack);
      }
    }
    for (int i = 54; i < 94; i++) {
      char    c = ' ';
      if ((*s) != '\0')
        c = *(s++);
      buf[i ^ 1] = uint8_t(c);
    }
    buf[94] = 0x80;             // maximum number of MULTIPLE sectors
    buf[95] = 0x80;
    buf[99] = 0x02;             // LBA is supported (DMA is not)
    buf[101] = 0x40;
    buf[106] = 0x01;            // current geometry and size below are valid
    buf[108] = uint8_t(nCylinders & 0x00FF);
    buf[109] = uint8_t(nCylinders >> 8);
    buf[110] = uint8_t(nHeads & 0x00FF);
    buf[111] = uint8_t(nHeads >> 8);
    buf[112] = uint8_t(nSectorsPerTrack & 0x00FF);
    buf[113] = uint8_t(nSectorsPerTrack >> 8);
    {
      uint32_t  nSectorsCHS =
          uint32_t(nCylinders) * uint32_t(nHeads) * uint32_t(nSectorsPerTrack);
      buf[114] = uint8_t(nSectorsCHS & 0xFFU);
      buf[115] = uint8_t((nSectorsCHS >> 8) & 0xFFU);
      buf[116] = uint8_t((nSectorsCHS >> 16) & 0xFFU);
      buf[117] = uint8_t((nSectorsCHS >> 24) & 0xFFU);
    }
    buf[118] = uint8_t(multSectCnt);
    buf[119] = 0x01;
    buf[120] = uint8_t(nSectors & 0xFFU);
    buf[121] = uint8_t((nSectors >> 8) & 0xFFU);
    buf[122] = uint8_t((nSectors >> 16) & 0xFFU);
    buf[123] = uint8_t((nSectors >> 24) & 0xFFU);
    buf[160] = 0x03;            // supports ATA-1 and ATA-2
    buf[166] = 0x10;            // supports removable media status notification
    buf[167] = 0x40;
    buf[172] = 0x10;            // removable media status notification enabled
    buf[175] = 0x40;
    buf[254] = 0x01;            // supports removable media status notification
    buf[510] = 0xA5;            // signature
    {
      uint8_t tmp = 0x00;
      for (int i = 0; i < 511; i++)
        tmp = (tmp - buf[i]) & 0xFF;
      buf[511] = tmp;           // checksum
    }
    sectorCnt = 0;
    // buffer is full, set DRQ flag
    readWordCnt = 256;
    bufPos = 0;
    ideController.statusRegister |= uint8_t(0x88);
  }

  void IDEInterface::IDEController::IDEDrive::initDeviceParametersCommand()
  {
    nHeads = (ideController.headRegister & 0x0F) + 1;
    nSectorsPerTrack = ideController.sectorCountRegister;
    if (nSectorsPerTrack > nHeads) {
      uint32_t  tmp = nSectors / (uint32_t(nHeads) * nSectorsPerTrack);
      if (tmp > nSectorsPerTrack && tmp <= 0xFFFFU) {
        nCylinders = uint16_t(tmp);
        diskChangeFlag = false;
        commandDone(0x00);
        return;
      }
    }
    // invalid disk geometry
    nCylinders = 0;
    nHeads = 0;
    nSectorsPerTrack = 0;
    commandDone(0x04);          // ABRT
  }

  void IDEInterface::IDEController::IDEDrive::readMultipleCommand()
  {
    if (!seekCommand())
      return;
    readBlock();
  }

  void IDEInterface::IDEController::IDEDrive::readSectorsCommand()
  {
    if (!seekCommand())
      return;
    readBlock();
  }

  void IDEInterface::IDEController::IDEDrive::readVerifySectorsCommand()
  {
    if (!seekCommand())
      return;
    do {
      readBlock();
    } while (sectorCnt > 0 && ideController.errorRegister == 0x00);
    ideController.statusRegister = (ideController.statusRegister & 0x77) | 0x80;
  }

  bool IDEInterface::IDEController::IDEDrive::seekCommand()
  {
    if (!(ideController.statusRegister & 0x40)) {
      commandDone(0x04);        // drive not ready: abort
      return false;
    }
    if (diskChangeFlag) {
      commandDone(0x24);        // media changed
      return false;
    }
    if (ideController.lbaMode) {
      currentSector = (uint32_t(ideController.headRegister & 0x0F) << 24)
                      | (uint32_t(ideController.cylinderRegister) << 8)
                      | uint32_t(ideController.sectorRegister);
    }
    else {
      convertCHSToLBA(currentSector,
                      ideController.cylinderRegister,
                      ideController.headRegister & 0x0F,
                      ideController.sectorRegister);
    }
    if (currentSector >= nSectors) {
      commandDone(ideController.commandRegister == 0x70 ? 0x10 : 0x14); // IDNF
      return false;
    }
    if (ideController.commandRegister == 0x70)      // SEEK
      commandDone(0x00);
    return true;
  }

  void IDEInterface::IDEController::IDEDrive::setFeaturesCommand()
  {
    // all flags are ignored; report success
    commandDone(0x00);
  }

  void IDEInterface::IDEController::IDEDrive::setMultipleModeCommand()
  {
    if (!(ideController.statusRegister & 0x40)) {
      commandDone(0x04);        // drive not ready: abort
      return;
    }
    if (ideController.sectorCountRegister < 2 ||
        ideController.sectorCountRegister > 128 ||
        (ideController.sectorCountRegister
         & (ideController.sectorCountRegister - 1)) != 0) {
      commandDone(0x04);        // invalid sector count
      return;
    }
    multSectCnt = ideController.sectorCountRegister;
    commandDone(0x00);
  }

  void IDEInterface::IDEController::IDEDrive::writeMultipleCommand()
  {
    if (readOnlyMode) {
      commandDone(0x04);        // read-only disk image: abort
      return;
    }
    if (!seekCommand())
      return;
    // buffer is empty, set DRQ flag
    writeWordCnt = multSectCnt << 8;
    bufPos = 0;
    ideController.statusRegister |= uint8_t(0x88);
  }

  void IDEInterface::IDEController::IDEDrive::writeSectorsCommand()
  {
    if (readOnlyMode) {
      commandDone(0x04);        // read-only disk image: abort
      return;
    }
    if (!seekCommand())
      return;
    // buffer is empty, set DRQ flag
    writeWordCnt = 256;
    bufPos = 0;
    ideController.statusRegister |= uint8_t(0x88);
  }

  void IDEInterface::IDEController::IDEDrive::writeVerifyCommand()
  {
    if (readOnlyMode) {
      commandDone(0x04);        // read-only disk image: abort
      return;
    }
    if (!seekCommand())
      return;
    // buffer is empty, set DRQ flag
    writeWordCnt = 256;
    bufPos = 0;
    ideController.statusRegister |= uint8_t(0x88);
  }

  IDEInterface::IDEController::IDEDrive::IDEDrive(IDEController& ideController_)
    : ideController(ideController_),
      imageFile((std::FILE *) 0),
      buf((uint8_t *) 0),
      nSectors(0U),
      nCylinders(0),
      nHeads(0),
      nSectorsPerTrack(0),
      multSectCnt(128),
      currentSector(0U),
      readWordCnt(0),
      writeWordCnt(0),
      sectorCnt(0),
      defaultCylinders(0),
      defaultHeads(0),
      defaultSectorsPerTrack(0),
      ledStateCounter(0),
      interruptFlag(false),
      readOnlyMode(true),
      diskChangeFlag(false),
      bufPos(0),
      vhdFormat(false)
  {
    this->reset(3);
  }

  IDEInterface::IDEController::IDEDrive::~IDEDrive()
  {
    setImageFile((char *) 0);
  }

  void IDEInterface::IDEController::IDEDrive::reset(int resetType)
  {
    if (resetType > 0) {
      nCylinders = defaultCylinders;
      nHeads = defaultHeads;
      nSectorsPerTrack = defaultSectorsPerTrack;
      multSectCnt = 128;
      diskChangeFlag = ((diskChangeFlag || resetType == 3) &&
                        (haveImageFile() && resetType != 2));
    }
    currentSector = 0U;
    readWordCnt = 0;
    writeWordCnt = 0;
    sectorCnt = 0;
    ledStateCounter = uint8_t(haveImageFile() ? 50 : 0);
    interruptFlag = false;
    bufPos = 0;
  }

  void IDEInterface::IDEController::IDEDrive::setImageFile(const char *fileName)
  {
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      if (imageFile) {
        std::fclose(imageFile);
        imageFile = (std::FILE *) 0;
      }
      nSectors = 0U;
      defaultCylinders = 0;
      defaultHeads = 0;
      defaultSectorsPerTrack = 0;
      readOnlyMode = true;
      vhdFormat = false;
      this->reset(3);
      return;
    }
    setImageFile((char *) 0);   // close any previously opened image file first
    try {
      imageFile = std::fopen(fileName, "r+b");
      if (!imageFile) {
        imageFile = std::fopen(fileName, "rb");
        if (!imageFile)
          throw Ep128Emu::Exception("error opening IDE disk image");
      }
      else {
        readOnlyMode = false;
      }
      (void) std::setvbuf(imageFile, (char *) 0, _IONBF, 0);
      nSectors = checkVHDImage(imageFile, fileName, defaultCylinders,
                               defaultHeads, defaultSectorsPerTrack);
      nCylinders = defaultCylinders;
      nHeads = defaultHeads;
      nSectorsPerTrack = defaultSectorsPerTrack;
      this->reset(3);
    }
    catch (...) {
      setImageFile((char *) 0);
      throw;
    }
  }

  uint16_t IDEInterface::IDEController::IDEDrive::readWord()
  {
    uint16_t  retval = uint16_t(buf[bufPos]) | (uint16_t(buf[bufPos + 1]) << 8);
    bufPos = bufPos + 2;
    if (--readWordCnt < 1)
      readBlock();
    ideController.dataRegister = retval;
    return retval;
  }

  void IDEInterface::IDEController::IDEDrive::writeWord()
  {
    uint16_t  n = ideController.dataRegister;
    buf[bufPos] = uint8_t(n & 0x00FF);
    buf[bufPos + 1] = uint8_t(n >> 8);
    bufPos = bufPos + 2;
    if (--writeWordCnt < 1)
      writeBlockDone();
  }

  void IDEInterface::IDEController::IDEDrive::processCommand()
  {
    if ((ideController.statusRegister & 0x80) != 0)
      return;                   // BUSY flag is still set, ignore command
    // abort any previous command, set BUSY flag, clear interrupt and error
    ideController.statusRegister = uint8_t(haveImageFile() ? 0xD0 : 0x80);
    ideController.errorRegister = 0x00;
    readWordCnt = 0;
    writeWordCnt = 0;
    sectorCnt = uint16_t((ideController.sectorCountRegister + 0xFF) & 0xFF) + 1;
    ledStateCounter = uint8_t(haveImageFile() ? 50 : 0);    // 100 ms
    interruptFlag = false;
    bufPos = 0;
    switch (ideController.commandRegister) {
    case 0x20:                  // READ SECTORS
    case 0x21:
      readSectorsCommand();
      break;
    case 0x30:                  // WRITE SECTORS
    case 0x31:
      writeSectorsCommand();
      break;
    case 0x3C:                  // WRITE VERIFY
      writeVerifyCommand();
      break;
    case 0x40:                  // READ VERIFY SECTORS
    case 0x41:
      readVerifySectorsCommand();
      break;
    case 0x70:                  // SEEK
      (void) seekCommand();
      break;
    case 0x91:                  // INITIALIZE DEVICE PARAMETERS
      initDeviceParametersCommand();
      break;
    case 0xC4:                  // READ MULTIPLE
      readMultipleCommand();
      break;
    case 0xC5:                  // WRITE MULTIPLE
      writeMultipleCommand();
      break;
    case 0xC6:                  // SET MULTIPLE MODE
      setMultipleModeCommand();
      break;
    case 0xDA:                  // GET MEDIA STATUS
      getMediaStatusCommand();
      break;
    case 0xDB:                  // ACKNOWLEDGE MEDIA CHANGE
      ackMediaChangeCommand();
      break;
    case 0xEC:                  // IDENTIFY DEVICE
      identifyDeviceCommand();
      break;
    case 0xEF:                  // SET FEATURES
      setFeaturesCommand();
      break;
    default:                    // any other command is invalid
      commandDone(0x04);        // ABRT
      break;
    }
  }

  void IDEInterface::IDEController::IDEDrive::commandDone(uint8_t errorCode)
  {
    errorCode = errorCode & 0x7F;
    switch (ideController.commandRegister) {
    case 0x20:                  // READ SECTORS
    case 0x21:
    case 0x30:                  // WRITE SECTORS
    case 0x31:
    case 0x3C:                  // WRITE VERIFY
    case 0x40:                  // READ VERIFY SECTORS
    case 0x41:
    case 0xC4:                  // READ MULTIPLE
    case 0xC5:                  // WRITE MULTIPLE
      ideController.sectorCountRegister = uint8_t(sectorCnt & 0x00FF);
      if (ideController.lbaMode) {
        ideController.sectorRegister = uint8_t(currentSector & 0xFFU);
        ideController.cylinderRegister =
            uint16_t((currentSector >> 8) & 0xFFFFU);
        ideController.headRegister = (ideController.headRegister & 0xF0)
                                     | uint8_t((currentSector >> 24) & 0x0FU);
      }
      else {
        uint16_t  h, s;
        convertLBAToCHS(ideController.cylinderRegister, h, s, currentSector);
        ideController.sectorRegister = uint8_t(s);
        ideController.headRegister = (ideController.headRegister & 0xF0)
                                     | (uint8_t(h) & 0x0F);
      }
      break;
    }
    readWordCnt = 0;
    writeWordCnt = 0;
    sectorCnt = 0;
    interruptFlag = interruptFlag | (!(ideController.statusRegister & 0x08));
    bufPos = 0;
    ideController.errorRegister = errorCode;
    ideController.statusRegister =
        (ideController.statusRegister & 0x50) | uint8_t(bool(errorCode));
  }

  // --------------------------------------------------------------------------

  void IDEInterface::IDEController::softwareReset()
  {
    this->reset(2);
  }

  IDEInterface::IDEController::IDEController()
    : ideDrive0(*this),
      ideDrive1(*this),
      buf((uint8_t *) 0),
      dataPort(0x0000),
      commandPort(0x30),
      statusRegister(0x00),
      dataRegister(0x0000),
      errorRegister(0x01),
      featuresRegister(0x00),
      sectorCountRegister(0x01),
      sectorRegister(0x01),
      cylinderRegister(0x0000),
      headRegister(0xE0),
      lbaMode(true),
      commandRegister(0x00),
      deviceControlRegister(0x00),
      currentDevice(&ideDrive0)
  {
    buf = new uint8_t[65536];
    std::memset(buf, 0x00, 65536);
    ideDrive0.setBuffer(buf);
    ideDrive1.setBuffer(buf);
  }

  IDEInterface::IDEController::~IDEController()
  {
    ideDrive0.setBuffer((uint8_t *) 0);
    ideDrive1.setBuffer((uint8_t *) 0);
    delete[] buf;
  }

  void IDEInterface::IDEController::reset(int resetType)
  {
    ideDrive0.reset(resetType);
    ideDrive1.reset(resetType);
    dataPort = 0x0000;
    commandPort = 0x30;
    statusRegister = uint8_t(ideDrive0.haveImageFile() ? 0x50 : 0x00);
    dataRegister = 0x0000;
    errorRegister = 0x01;
    featuresRegister = 0x00;
    sectorCountRegister = 0x01;
    sectorRegister = 0x01;
    cylinderRegister = 0x0000;
    headRegister = 0xE0;
    lbaMode = true;
    commandRegister = 0x00;
    deviceControlRegister = 0x00;
    currentDevice = &ideDrive0;
  }

  void IDEInterface::IDEController::setImageFile(int n, const char *fileName)
  {
    try {
      if ((n & 1) == 0)
        ideDrive0.setImageFile(fileName);
      else
        ideDrive1.setImageFile(fileName);
    }
    catch (...) {
      if (int((headRegister & 0x10) >> 4) == (n & 1))
        this->reset(3);
      throw;
    }
    if (int((headRegister & 0x10) >> 4) == (n & 1))
      this->reset(3);
  }

  void IDEInterface::IDEController::readRegister()
  {
    if ((commandPort & 0x10) != 0) {
      // DIOR- is not active
      return;
    }
    int     n = commandPort & 0x0F;
    if ((statusRegister & 0x80) != 0 && n < 8)
      n = 7;                    // if BUSY flag is set, only status can be read
    switch (n) {
    case 0:                     // data
      dataPort = dataRegister;
      break;
    case 1:                     // error
      dataPort = errorRegister;
      break;
    case 2:                     // sector count
      dataPort = sectorCountRegister;
      break;
    case 3:                     // sector number / LBA(7:0)
      dataPort = sectorRegister;
      break;
    case 4:                     // cylinder low / LBA(15:8)
      dataPort = cylinderRegister;
      break;
    case 5:                     // cylinder high / LBA(23:16)
      dataPort = cylinderRegister >> 8;
      break;
    case 6:                     // device / head / LBA(27:24)
      dataPort = headRegister | 0xA0;
      break;
    case 7:                     // status
      dataPort = statusRegister;
      break;
    case 14:                    // alternate status (does not clear INTRQ)
      dataPort = statusRegister;
      break;
    case 15:                    // device address (obsolete)
      dataPort = uint16_t((headRegister & 0x10) == 0 ? 0x7E : 0x7D);
      break;
    default:                    // anything else is invalid
      dataPort = 0x0000;
      break;
    }
    if (n != 0)                 // all registers other than DATA are 8-bit
      dataPort = dataPort & 0x00FF;
  }

  void IDEInterface::IDEController::writeRegister()
  {
    int     n = commandPort & 0x0F;
    if ((n & 8) == 0 && (statusRegister & 0x80) != 0)
      return;           // ignore writes to command block if BUSY flag is set
    if (n >= 1 && n <= 6 && (statusRegister & 0x08) != 0) {
      // if DRQ is set, and writing to command block
      // registers (other than DATA), abort the command
      currentDevice->commandDone(errorRegister | 0x04); // ABRT
    }
    uint8_t value = uint8_t(dataPort & 0x00FF);         // for 8-bit registers
    switch (n) {
    case 0:             // data
      dataRegister = dataPort;
      break;
    case 1:             // features
      featuresRegister = value;
      break;
    case 2:             // sector count
      sectorCountRegister = value;
      break;
    case 3:             // sector number / LBA(7:0)
      sectorRegister = value;
      break;
    case 4:             // cylinder low / LBA(15:8)
      cylinderRegister = (cylinderRegister & 0xFF00) | uint16_t(value);
      break;
    case 5:             // cylinder high / LBA(23:16)
      cylinderRegister = (cylinderRegister & 0x00FF) | (uint16_t(value) << 8);
      break;
    case 6:             // device / head / LBA(27:24)
      headRegister = value | 0xA0;
      if ((value & 0x10) == 0)
        currentDevice = &ideDrive0;
      else
        currentDevice = &ideDrive1;
      lbaMode = bool(value & 0x40);
      break;
    case 7:             // command
      currentDevice->interruptFlag = false;
      commandRegister = value;
      break;
    case 14:            // device control
      deviceControlRegister = value;
      if ((deviceControlRegister & 0x04) != 0)  // software reset
        this->softwareReset();
      break;
    }
  }

  // --------------------------------------------------------------------------

  uint32_t IDEInterface::getLEDState_()
  {
    uint32_t  retval = 0U;
    if (idePort0.ideDrive0.ledStateCounter > 0) {
      if (!idePort0.ideDrive0.getIsBusy())
        idePort0.ideDrive0.ledStateCounter--;
      retval = 0x00000004U;
    }
    if (idePort0.ideDrive1.ledStateCounter > 0) {
      if (!idePort0.ideDrive1.getIsBusy())
        idePort0.ideDrive1.ledStateCounter--;
      retval = retval | 0x00000400U;
    }
    if (idePort1.ideDrive0.ledStateCounter > 0) {
      if (!idePort1.ideDrive0.getIsBusy())
        idePort1.ideDrive0.ledStateCounter--;
      retval = retval | 0x00040000U;
    }
    if (idePort1.ideDrive1.ledStateCounter > 0) {
      if (!idePort1.ideDrive1.getIsBusy())
        idePort1.ideDrive1.ledStateCounter--;
      retval = retval | 0x04000000U;
    }
    if ((ledFlashCnt & 0x20) != 0)
      retval = retval | (retval << 1);
    return retval;
  }

  IDEInterface::IDEInterface()
    : idePort0(),
      idePort1(),
      dataPort(0x0000),
      ledFlashCnt(0x00)
  {
  }

  IDEInterface::~IDEInterface()
  {
  }

  void IDEInterface::reset(int resetType)
  {
    dataPort = 0x0000;
    idePort0.reset(resetType);
    idePort1.reset(resetType);
  }

  void IDEInterface::setImageFile(int n, const char *fileName)
  {
    if ((n & 2) == 0)
      idePort0.setImageFile(n, fileName);
    else
      idePort1.setImageFile(n, fileName);
  }

  uint8_t IDEInterface::readPort(uint16_t addr)
  {
    switch (addr & 3) {
    case 0:
      return uint8_t(dataPort & 0x00FF);
    case 1:
      return uint8_t(dataPort >> 8);
    case 2:
      idePort1.commandPort = (idePort1.commandPort & 0x7F)
                             | (uint8_t(idePort1.getInterruptFlag()) << 7);
      return uint8_t((idePort1.commandPort & 0xFC)
                     | ((idePort1.commandPort & 0x01) << 1)
                     | ((idePort1.commandPort & 0x02) >> 1));
    case 3:
      idePort0.commandPort = (idePort0.commandPort & 0x7F)
                             | (uint8_t(idePort0.getInterruptFlag()) << 7);
      return uint8_t((idePort0.commandPort & 0xFC)
                     | ((idePort0.commandPort & 0x01) << 1)
                     | ((idePort0.commandPort & 0x02) >> 1));
    }
    return 0x00;        // not reached
  }

  void IDEInterface::writePort(uint16_t addr, uint8_t value)
  {
    if ((idePort1.commandPort & 0x20) == 0)
      idePort1.dataPort = dataPort;
    if ((idePort0.commandPort & 0x20) == 0)
      idePort0.dataPort = dataPort;
    if ((addr & 2) == 0) {
      if ((addr & 1) == 0)
        dataPort = (dataPort & 0xFF00) | uint16_t(value);
      else
        dataPort = (dataPort & 0x00FF) | (uint16_t(value) << 8);
      return;
    }
    IDEController&  idePort = ((addr & 1) == 0 ? idePort1 : idePort0);
    IDEController::IDEDrive&  ideDrive = idePort.getCurrentDevice();
    uint8_t&  commandPort = idePort.commandPort;
    int     registerRead = -1;
    int     registerWritten = -1;
    if ((value & 0x10) > (commandPort & 0x10))
      registerRead = commandPort & 0x0F;
    if ((value & 0x20) > (commandPort & 0x20))
      registerWritten = commandPort & 0x0F;
    commandPort = uint8_t((commandPort & 0xC0) | (value & 0x3C)
                          | ((value & 0x01) << 1) | ((value & 0x02) >> 1));
    if ((commandPort & 0x10) == 0) {
      idePort.readRegister();
      dataPort = idePort.dataPort;
    }
    if (registerRead >= 0) {
      if (registerRead == 0) {
        if (ideDrive.isReadCommand() && idePort.checkDRQFlag()) {
          // read block
          idePort.dataPort = ideDrive.readWord();
        }
        else {
          idePort.dataPort = 0x0000;    // invalid read of data register
        }
        dataPort = idePort.dataPort;
      }
      else if (registerRead == 7 || registerRead == 14) {
        idePort.checkBSYFlag();
        (void) idePort.checkDRQFlag();
        if (registerRead == 7) {
          // clear interrupt on reading the status register
          ideDrive.interruptFlag = false;
        }
      }
    }
    if (registerWritten >= 0) {
      idePort.writeRegister();
      if (registerWritten == 0) {
        if (ideDrive.isWriteCommand() && idePort.checkDRQFlag()) {
          // write block
          ideDrive.writeWord();
        }
      }
      else if (registerWritten == 7) {
        // if command register was written, then execute command
        ideDrive.processCommand();
      }
    }
  }

}       // namespace Ep128

