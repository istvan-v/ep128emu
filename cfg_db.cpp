
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

#include "ep128emu.hpp"
#include "fileio.hpp"
#include "system.hpp"
#include "cfg_db.hpp"
#include <map>
#include <cmath>

namespace Ep128Emu {

  class ConfigurationVariable_Boolean
    : public ConfigurationDB::ConfigurationVariable {
   private:
    bool&   value;
   public:
    ConfigurationVariable_Boolean(bool& ref)
      : ConfigurationDB::ConfigurationVariable(),
        value(ref)
    {
    }
    virtual ~ConfigurationVariable_Boolean()
    {
    }
    virtual operator bool()
    {
      return value;
    }
    virtual void operator=(const bool& n)
    {
      value = n;
    }
  };

  class ConfigurationVariable_Integer
    : public ConfigurationDB::ConfigurationVariable {
   private:
    int&    value;
    int     minValue;
    int     maxValue;
    int     step;
    bool    powOfTwoFlag;
   public:
    ConfigurationVariable_Integer(int& ref)
      : ConfigurationDB::ConfigurationVariable(),
        value(ref)
    {
      minValue = (-1 - int(0x7FFFFFFF));
      maxValue = 0x7FFFFFFF;
      step = 1;
      powOfTwoFlag = false;
      checkValue();
    }
    virtual ~ConfigurationVariable_Integer()
    {
    }
    virtual operator int()
    {
      return value;
    }
    virtual void operator=(const int& n)
    {
      value = n;
      checkValue();
    }
    virtual void setRange(double min, double max, double step_)
    {
      minValue = (min >= 0.0 ?
                  (min < 2147483646.5 ? int(min + 0.5) : 2147483647)
                  : (min > -2147483647.5 ? int(min - 0.5) : (-1 - 2147483647)));
      maxValue = (max >= 0.0 ?
                  (max < 2147483646.5 ? int(max + 0.5) : 2147483647)
                  : (max > -2147483647.5 ? int(max - 0.5) : (-1 - 2147483647)));
      if (minValue >= maxValue) {
        minValue = (-1 - 2147483647);
        maxValue = 2147483647;
      }
      step = (step_ >= 0.5 && step_ < 2147483647.5 ? int(step_ + 0.5) : 1);
      checkValue();
    }
    virtual void setRequirePowerOfTwo(bool n)
    {
      powOfTwoFlag = n;
      checkValue();
    }
    virtual void checkValue()
    {
      if (step > 1) {
        if (value >= 0)
          value = ((value + (step >> 1)) / step) * step;
        else
          value = ((value - (step >> 1)) / step) * step;
      }
      if (powOfTwoFlag) {
        if (value < 1 || (value & (value - 1)) != 0) {
          int     i;
          for (i = 1; i < value && i < 0x40000000; i <<= 1)
            ;
          value = i;
        }
      }
      if (value < minValue)
        value = minValue;
      else if (value > maxValue)
        value = maxValue;
    }
  };

  class ConfigurationVariable_UnsignedInteger
    : public ConfigurationDB::ConfigurationVariable {
   private:
    unsigned int& value;
    unsigned int  minValue;
    unsigned int  maxValue;
    unsigned int  step;
    bool          powOfTwoFlag;
   public:
    ConfigurationVariable_UnsignedInteger(unsigned int& ref)
      : ConfigurationDB::ConfigurationVariable(),
        value(ref)
    {
      minValue = 0U;
      maxValue = 0xFFFFFFFFU;
      step = 1U;
      powOfTwoFlag = false;
      checkValue();
    }
    virtual ~ConfigurationVariable_UnsignedInteger()
    {
    }
    virtual operator unsigned int()
    {
      return value;
    }
    virtual void operator=(const unsigned int& n)
    {
      value = n;
      checkValue();
    }
    virtual void setRange(double min, double max, double step_)
    {
      minValue = (min > 0.0 ?
                  (min < 4294967294.5 ?
                   (unsigned int) (min + 0.5) : 0xFFFFFFFFU)
                  : 0U);
      maxValue = (max > 0.0 ?
                  (max < 4294967294.5 ?
                   (unsigned int) (max + 0.5) : 0xFFFFFFFFU)
                  : 0U);
      if (minValue >= maxValue) {
        minValue = 0U;
        maxValue = 0xFFFFFFFFU;
      }
      step = (step_ >= 0.5 && step_ < 4294967295.5 ?
              (unsigned int) (step_ + 0.5) : 1);
      checkValue();
    }
    virtual void setRequirePowerOfTwo(bool n)
    {
      powOfTwoFlag = n;
      checkValue();
    }
    virtual void checkValue()
    {
      if (step > 1U) {
        value = ((value + (step >> 1)) / step) * step;
      }
      if (powOfTwoFlag) {
        if (value < 1U || (value & (value - 1U)) != 0U) {
          unsigned int  i;
          for (i = 1U; i < value && i < 0x80000000U; i <<= 1)
            ;
          value = i;
        }
      }
      if (value < minValue)
        value = minValue;
      else if (value > maxValue)
        value = maxValue;
    }
  };

  class ConfigurationVariable_Float
    : public ConfigurationDB::ConfigurationVariable {
   private:
    double& value;
    double  minValue;
    double  maxValue;
    double  step;
   public:
    ConfigurationVariable_Float(double& ref)
      : ConfigurationDB::ConfigurationVariable(),
        value(ref)
    {
      minValue = -2147483648.0;
      maxValue =  2147483647.999999;
      step = 0.0;
      checkValue();
    }
    virtual ~ConfigurationVariable_Float()
    {
    }
    virtual operator double()
    {
      return value;
    }
    virtual void operator=(const double& n)
    {
      value = n;
      checkValue();
    }
    virtual void setRange(double min, double max, double step_)
    {
      minValue = (min >= -2147483648.0      ? min : -2147483648.0);
      maxValue = (max <=  2147483647.999999 ? max :  2147483647.999999);
      if (minValue >= maxValue) {
        minValue = -2147483648.0;
        maxValue =  2147483647.999999;
      }
      step = (step_ >= 0.000000001 && step_ <= 2147483647.999999 ? step_ : 0.0);
      checkValue();
    }
    virtual void checkValue()
    {
      if (step > 0.0) {
        double  tmp = (value + (step * 0.5)) / step;
        value = std::floor(tmp) * step;
      }
      if (value < minValue)
        value = minValue;
      else if (value > maxValue)
        value = maxValue;
      else if (!(value >= minValue && value <= maxValue))
        value = (minValue + maxValue) * 0.5;
    }
  };

  class ConfigurationVariable_String
    : public ConfigurationDB::ConfigurationVariable {
   private:
     std::string& value;
     bool         stripStringFlag;
     bool         lowerCaseFlag;
     bool         upperCaseFlag;
   public:
    ConfigurationVariable_String(std::string& ref)
      : ConfigurationDB::ConfigurationVariable(),
        value(ref)
    {
      stripStringFlag = false;
      lowerCaseFlag = false;
      upperCaseFlag = false;
    }
    virtual ~ConfigurationVariable_String()
    {
    }
    virtual operator std::string()
    {
      return value;
    }
    virtual void operator=(const std::string& n)
    {
      value = n;
      checkValue();
    }
    virtual void setStripString(bool n)
    {
      stripStringFlag = n;
      if (n) {
        checkValue();
      }
    }
    virtual void setStringToLowerCase(bool n)
    {
      lowerCaseFlag = n;
      if (n) {
        upperCaseFlag = false;
        checkValue();
      }
    }
    virtual void setStringToUpperCase(bool n)
    {
      upperCaseFlag = n;
      if (n) {
        lowerCaseFlag = false;
        checkValue();
      }
    }
    virtual void checkValue()
    {
      if (stripStringFlag)
        stripString(value);
      if (lowerCaseFlag)
        stringToLowerCase(value);
      if (upperCaseFlag)
        stringToUpperCase(value);
    }
  };

  // --------------------------------------------------------------------------

  ConfigurationDB::~ConfigurationDB()
  {
    std::map<std::string, ConfigurationVariable *>::iterator  i;

    for (i = db.begin(); i != db.end(); i++)
      delete (*i).second;
    db.clear();
  }

  ConfigurationDB::ConfigurationVariable&
      ConfigurationDB::operator[](const std::string& keyName)
  {
    std::map<std::string, ConfigurationVariable *>::iterator  i;

    i = db.find(keyName);
    if (i == db.end())
      throw Exception("configuration variable is not found");
    return *((*i).second);
  }

  void ConfigurationDB::createKey(const std::string& name, bool& ref)
  {
    if (db.find(name) != db.end())
      throw Exception("cannot create configuration variable: "
                      "the key name is already in use");
    ConfigurationVariable_Boolean   *p;
    p = new ConfigurationVariable_Boolean(ref);
    try {
      db.insert(std::pair<std::string, ConfigurationVariable *>(name, p));
    }
    catch (...) {
      delete p;
      throw;
    }
  }

  void ConfigurationDB::createKey(const std::string& name, int& ref)
  {
    if (db.find(name) != db.end())
      throw Exception("cannot create configuration variable: "
                      "the key name is already in use");
    ConfigurationVariable_Integer   *p;
    p = new ConfigurationVariable_Integer(ref);
    try {
      db.insert(std::pair<std::string, ConfigurationVariable *>(name, p));
    }
    catch (...) {
      delete p;
      throw;
    }
  }

  void ConfigurationDB::createKey(const std::string& name, unsigned int& ref)
  {
    if (db.find(name) != db.end())
      throw Exception("cannot create configuration variable: "
                      "the key name is already in use");
    ConfigurationVariable_UnsignedInteger   *p;
    p = new ConfigurationVariable_UnsignedInteger(ref);
    try {
      db.insert(std::pair<std::string, ConfigurationVariable *>(name, p));
    }
    catch (...) {
      delete p;
      throw;
    }
  }

  void ConfigurationDB::createKey(const std::string& name, double& ref)
  {
    if (db.find(name) != db.end())
      throw Exception("cannot create configuration variable: "
                      "the key name is already in use");
    ConfigurationVariable_Float     *p;
    p = new ConfigurationVariable_Float(ref);
    try {
      db.insert(std::pair<std::string, ConfigurationVariable *>(name, p));
    }
    catch (...) {
      delete p;
      throw;
    }
  }

  void ConfigurationDB::createKey(const std::string& name, std::string& ref)
  {
    if (db.find(name) != db.end())
      throw Exception("cannot create configuration variable: "
                      "the key name is already in use");
    ConfigurationVariable_String    *p;
    p = new ConfigurationVariable_String(ref);
    try {
      db.insert(std::pair<std::string, ConfigurationVariable *>(name, p));
    }
    catch (...) {
      delete p;
      throw;
    }
  }

  // --------------------------------------------------------------------------

  class ChunkType_ConfigDB : public File::ChunkTypeHandler {
   private:
    ConfigurationDB&  ref;
   public:
    ChunkType_ConfigDB(ConfigurationDB& ref_)
      : File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_ConfigDB()
    {
    }
    virtual File::ChunkType getChunkType() const
    {
      return File::EP128EMU_CHUNKTYPE_CONFIG_DB;
    }
    virtual void processChunk(File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void ConfigurationDB::saveState(File::Buffer& buf)
  {
    std::map<std::string, ConfigurationVariable *>::iterator  i;

    buf.setPosition(0);
    for (i = db.begin(); i != db.end(); i++) {
      ConfigurationVariable&  cv = *((*i).second);
      if (typeid(cv) == typeid(ConfigurationVariable_Boolean)) {
        buf.writeUInt32(0x00000001);
        buf.writeString((*i).first);
        buf.writeBoolean(bool(cv));
      }
      else if (typeid(cv) == typeid(ConfigurationVariable_Integer)) {
        buf.writeUInt32(0x00000002);
        buf.writeString((*i).first);
        buf.writeInt32(int(cv));
      }
      else if (typeid(cv) == typeid(ConfigurationVariable_UnsignedInteger)) {
        buf.writeUInt32(0x00000003);
        buf.writeString((*i).first);
        buf.writeUInt32((unsigned int) cv);
      }
      else if (typeid(cv) == typeid(ConfigurationVariable_Float)) {
        buf.writeUInt32(0x00000004);
        buf.writeString((*i).first);
        buf.writeFloat(double(cv));
      }
      else if (typeid(cv) == typeid(ConfigurationVariable_String)) {
        buf.writeUInt32(0x00000005);
        buf.writeString((*i).first);
        buf.writeString(std::string(cv));
      }
    }
  }

  void ConfigurationDB::saveState(File& f)
  {
    File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(File::EP128EMU_CHUNKTYPE_CONFIG_DB, buf);
  }

  void ConfigurationDB::loadState(File::Buffer& buf)
  {
    buf.setPosition(0);
    while (buf.getPosition() < buf.getDataSize()) {
      int           type;
      std::string   name;
      std::map<std::string, ConfigurationVariable *>::iterator  i;
      ConfigurationVariable   *cv;

      type = int(buf.readUInt32());
      if (type < 1 || type > 5)
        throw Exception("unknown configuration variable type");
      name = buf.readString();
      i = db.find(name);
      if (i != db.end())
        cv = (*i).second;
      else
        cv = (ConfigurationVariable *) 0;
      switch (type) {
      case 0x00000001:
        {
          bool    value = buf.readBoolean();
          if (cv) {
            if (typeid(*cv) == typeid(ConfigurationVariable_Boolean))
              *cv = value;
          }
        }
        break;
      case 0x00000002:
        {
          int     value = buf.readInt32();
          if (cv) {
            if (typeid(*cv) == typeid(ConfigurationVariable_Integer))
              *cv = value;
          }
        }
        break;
      case 0x00000003:
        {
          unsigned int  value = buf.readUInt32();
          if (cv) {
            if (typeid(*cv) == typeid(ConfigurationVariable_UnsignedInteger))
              *cv = value;
          }
        }
        break;
      case 0x00000004:
        {
          double  value = buf.readFloat();
          if (cv) {
            if (typeid(*cv) == typeid(ConfigurationVariable_Float))
              *cv = value;
          }
        }
        break;
      case 0x00000005:
        {
          std::string   value = buf.readString();
          if (cv) {
            if (typeid(*cv) == typeid(ConfigurationVariable_String))
              *cv = value;
          }
        }
        break;
      }
    }
  }

  void ConfigurationDB::registerChunkType(File& f)
  {
    ChunkType_ConfigDB  *p;
    p = new ChunkType_ConfigDB(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

  // --------------------------------------------------------------------------

  ConfigurationDB::ConfigurationVariable::~ConfigurationVariable()
  {
  }

  ConfigurationDB::ConfigurationVariable::operator bool()
  {
    throw Exception("configuration variable is not of type 'Boolean'");
    return false;
  }

  ConfigurationDB::ConfigurationVariable::operator int()
  {
    throw Exception("configuration variable is not of type 'Integer'");
    return 0;
  }

  ConfigurationDB::ConfigurationVariable::operator unsigned int()
  {
    throw Exception("configuration variable is not of type 'Unsigned Integer'");
    return 0U;
  }

  ConfigurationDB::ConfigurationVariable::operator double()
  {
    throw Exception("configuration variable is not of type 'Float'");
    return 0.0;
  }

  ConfigurationDB::ConfigurationVariable::operator std::string()
  {
    throw Exception("configuration variable is not of type 'String'");
    return std::string("");
  }

  void ConfigurationDB::ConfigurationVariable::operator=(const bool& n)
  {
    (void) n;
    throw Exception("configuration variable is not of type 'Boolean'");
  }

  void ConfigurationDB::ConfigurationVariable::operator=(const int& n)
  {
    (void) n;
    throw Exception("configuration variable is not of type 'Integer'");
  }

  void ConfigurationDB::ConfigurationVariable::operator=(const unsigned int& n)
  {
    (void) n;
    throw Exception("configuration variable is not of type 'Unsigned Integer'");
  }

  void ConfigurationDB::ConfigurationVariable::operator=(const double& n)
  {
    (void) n;
    throw Exception("configuration variable is not of type 'Float'");
  }

  void ConfigurationDB::ConfigurationVariable::operator=(const std::string& n)
  {
    (void) n;
    throw Exception("configuration variable is not of type 'String'");
  }

  void ConfigurationDB::ConfigurationVariable::setRange(double min, double max,
                                                        double step)
  {
    (void) min;
    (void) max;
    (void) step;
    throw Exception("cannot set range for configuration variable");
  }

  void ConfigurationDB::ConfigurationVariable::setRequirePowerOfTwo(bool n)
  {
    (void) n;
    throw Exception("cannot set 'power of two' flag "
                    "for configuration variable");
  }

  void ConfigurationDB::ConfigurationVariable::setStripString(bool n)
  {
    (void) n;
    throw Exception("cannot set 'strip string' flag "
                    "for configuration variable");
  }

  void ConfigurationDB::ConfigurationVariable::setStringToLowerCase(bool n)
  {
    (void) n;
    throw Exception("cannot set 'string to lower case' flag "
                    "for configuration variable");
  }

  void ConfigurationDB::ConfigurationVariable::setStringToUpperCase(bool n)
  {
    (void) n;
    throw Exception("cannot set 'string to upper case' flag "
                    "for configuration variable");
  }

  void ConfigurationDB::ConfigurationVariable::checkValue()
  {
  }

}       // namespace Ep128Emu

