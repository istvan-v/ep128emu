
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EP128EMU_CFG_DB_HPP
#define EP128EMU_CFG_DB_HPP

#include "ep128emu.hpp"
#include "fileio.hpp"
#include <map>

namespace Ep128Emu {

  class ConfigurationDB {
   public:
    class ConfigurationVariable {
     protected:
      const std::string name;
      void    *callbackUserData;
      bool    callbackOnChangeOnly;
     public:
      ConfigurationVariable(const std::string& name_)
        : name(name_),
          callbackUserData((void *) 0),
          callbackOnChangeOnly(false)
      {
      }
      virtual ~ConfigurationVariable();
      virtual operator bool();
      virtual operator int();
      virtual operator unsigned int();
      virtual operator double();
      virtual operator std::string();
      virtual void operator=(const bool&);
      virtual void operator=(const int&);
      virtual void operator=(const unsigned int&);
      virtual void operator=(const double&);
      virtual void operator=(const char *);
      virtual void operator=(const std::string&);
      virtual void setRange(double min, double max, double step = 0.0);
      virtual void setRequirePowerOfTwo(bool);
      virtual void setStripString(bool);
      virtual void setStringToLowerCase(bool);
      virtual void setStringToUpperCase(bool);
      virtual void setCallback(void (*func)(void *userData_,
                                            const std::string& name_,
                                            bool value_),
                               void *userData, bool callOnChangeOnly = true);
      virtual void setCallback(void (*func)(void *userData_,
                                            const std::string& name_,
                                            int value_),
                               void *userData, bool callOnChangeOnly = true);
      virtual void setCallback(void (*func)(void *userData_,
                                            const std::string& name_,
                                            unsigned int value_),
                               void *userData, bool callOnChangeOnly = true);
      virtual void setCallback(void (*func)(void *userData_,
                                            const std::string& name_,
                                            double value_),
                               void *userData, bool callOnChangeOnly = true);
      virtual void setCallback(void (*func)(void *userData_,
                                            const std::string& name_,
                                            const std::string& value_),
                               void *userData, bool callOnChangeOnly = true);
     protected:
      virtual void checkValue();
    };
   private:
    std::map<std::string, ConfigurationVariable *>  db;
   public:
    ConfigurationDB()
    {
    }
    ~ConfigurationDB();
    ConfigurationVariable& operator[](const std::string&);
    void createKey(const std::string& name, bool& ref);
    void createKey(const std::string& name, int& ref);
    void createKey(const std::string& name, unsigned int& ref);
    void createKey(const std::string& name, double& ref);
    void createKey(const std::string& name, std::string& ref);
    void saveState(File::Buffer& buf);
    void saveState(File& f);
    void saveState(const char *fileName,
                   bool useHomeDirectory = false);  // save in ASCII format
    void loadState(File::Buffer& buf);
    void loadState(const char *fileName,
                   bool useHomeDirectory = false);  // load ASCII format file
    void registerChunkType(File& f);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_CFG_DB_HPP

