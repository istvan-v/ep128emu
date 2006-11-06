
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

#include "ep128.hpp"
#include "fileio.hpp"
#include <map>

namespace Ep128Emu {

  class ConfigurationDB {
   public:
    class ConfigurationVariable {
     public:
      ConfigurationVariable()
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
      virtual void operator=(const std::string&);
      virtual void setRange(double min, double max, double step = 0.0);
      virtual void setRequirePowerOfTwo(bool);
      virtual void setStripString(bool);
      virtual void setStringToLowerCase(bool);
      virtual void setStringToUpperCase(bool);
     private:
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
    void saveState(Ep128::File::Buffer& buf);
    void loadState(Ep128::File::Buffer& buf);
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_CFG_DB_HPP

