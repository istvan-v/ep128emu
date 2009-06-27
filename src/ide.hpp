
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2009 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EP128EMU_IDE_HPP
#define EP128EMU_IDE_HPP

#include "ep128emu.hpp"

namespace Ep128 {

  class IDEInterface {
   protected:
    class IDEController {
     public:
      class IDEDrive {
       protected:
        IDEController&  ideController;
        std::FILE *imageFile;
        uint8_t   *buf;         // 512 bytes
        uint32_t  nSectors;
        uint16_t  nCylinders;
        uint16_t  nHeads;
        uint16_t  nSectorsPerTrack;
        uint16_t  multSectCnt;
        uint32_t  currentSector;
        uint16_t  readWordCnt;
        uint16_t  writeWordCnt;
        uint16_t  sectorCnt;
        uint16_t  defaultCylinders;
        uint16_t  defaultHeads;
        uint16_t  defaultSectorsPerTrack;
       public:
        uint8_t   ledStateCounter;
        bool      interruptFlag;
       protected:
        bool      readOnlyMode;
        bool      vhdFormat;
        // --------
        bool calculateCHS(uint16_t& c, uint16_t& h, uint16_t& s);
        bool convertCHSToLBA(uint32_t& b, uint16_t c, uint16_t h, uint16_t s);
        bool convertLBAToCHS(uint16_t& c, uint16_t& h, uint16_t& s, uint32_t b);
        void commandDone(uint8_t errorCode);
        void readBlock();
        void writeBlockDone();
        void softwareReset();
        void identifyDeviceCommand();
        void initDeviceParametersCommand();
        void readMultipleCommand();
        void readSectorsCommand();
        void readVerifySectorsCommand();
        // NOTE: this function is also called by other commands
        // to set the disk position; returns true if the seek was successful
        bool seekCommand();
        void setFeaturesCommand();
        void setMultipleModeCommand();
        void writeMultipleCommand();
        void writeSectorsCommand();
        void writeVerifyCommand();
       public:
        IDEDrive(IDEController& ideController_);
        virtual ~IDEDrive();
        void reset(bool resetParameters);
        void setImageFile(const char *fileName);
        uint16_t readWord();
        void writeWord();
        void processCommand();
        inline bool getIsBusy() const
        {
          return ((ideController.statusRegister & 0x80) != 0 &&
                  ideController.currentDevice == this);
        }
        inline bool haveImageFile() const
        {
          return (imageFile != (std::FILE *) 0);
        }
        inline bool isReadCommand() const
        {
          return (readWordCnt > 0);
        }
        inline bool isWriteCommand() const
        {
          return (writeWordCnt > 0);
        }
      };
      // --------
      IDEDrive  ideDrive0;
      IDEDrive  ideDrive1;
      uint16_t  dataPort;
      uint8_t   commandPort;
     protected:
      uint8_t   statusRegister;
      uint16_t  dataRegister;
      uint8_t   errorRegister;
      uint8_t   featuresRegister;
      uint8_t   sectorCountRegister;
      uint8_t   sectorRegister;
      uint16_t  cylinderRegister;
      // bit 7: always 1
      // bit 6: LBA mode (1: yes)
      // bit 5: always 1
      // bit 4: device select
      // bit 3..0: head / LBA(27:24)
      uint8_t   headRegister;
      bool      lbaMode;
      uint8_t   commandRegister;
      uint8_t   deviceControlRegister;
      IDEDrive  *currentDevice;
      // --------
      void softwareReset();
     public:
      IDEController();
      virtual ~IDEController();
      void reset(bool resetParameters);
      void setImageFile(int n, const char *fileName);
      void readRegister();
      void writeRegister();
      inline IDEDrive& getCurrentDevice()
      {
        return (*currentDevice);
      }
      inline bool getInterruptFlag() const
      {
        if ((deviceControlRegister & 0x02) == 0)
          return currentDevice->interruptFlag;
        return false;
      }
      inline bool checkDRQFlag()
      {
        if ((statusRegister & 0x08) != 0) {
          statusRegister = statusRegister & 0x7F;
          return true;
        }
        return false;
      }
    };
    // --------
    IDEController idePort0;     // primary controller at EFh
    IDEController idePort1;     // secondary controller at EEh
    uint16_t  dataPort;
    uint8_t   ledFlashCnt;
    // --------
    uint32_t getLEDState_();
   public:
    IDEInterface();
    virtual ~IDEInterface();
    void reset(bool resetParameters);
    void setImageFile(int n, const char *fileName);
    uint8_t readPort(uint16_t addr);
    void writePort(uint16_t addr, uint8_t value);
    inline uint32_t getLEDState()
    {
      ledFlashCnt++;
      if ((idePort0.ideDrive0.ledStateCounter
           | idePort0.ideDrive1.ledStateCounter
           | idePort1.ideDrive0.ledStateCounter
           | idePort1.ideDrive1.ledStateCounter) != 0) {
        return getLEDState_();
      }
      return 0U;
    }
  };

}       // namespace Ep128

#endif  // EP128EMU_IDE_HPP

