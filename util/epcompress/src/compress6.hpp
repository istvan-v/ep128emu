
// compressor utility for Enterprise 128 programs
// Copyright (C) 2007-2018 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifndef EPCOMPRESS_COMPRESS6_HPP
#define EPCOMPRESS_COMPRESS6_HPP

#include "ep128emu.hpp"
#include "compress.hpp"
#include "comprlib.hpp"
#include "decompress6.hpp"

#include <vector>

namespace Ep128Compress {

  class Compressor_M6 : public Compressor {
   private:
    static const size_t minRepeatDist = 1;
    static const size_t maxRepeatDist = 524288;
    static const size_t minRepeatLen = 1;
    static const size_t maxRepeatLen = 512;
    static const unsigned int lengthMaxValue = 65535U;
    // --------
    struct LZMatchParameters {
      unsigned int  d;
      unsigned int  len;
      LZMatchParameters()
        : d(0U),
          len(1U)
      {
      }
      LZMatchParameters(const LZMatchParameters& r)
        : d(r.d),
          len(r.len)
      {
      }
      ~LZMatchParameters()
      {
      }
      inline LZMatchParameters& operator=(const LZMatchParameters& r)
      {
        d = r.d;
        len = r.len;
        return (*this);
      }
      inline void clear()
      {
        d = 0U;
        len = 1U;
      }
    };
    // --------
    LZSearchTable   *searchTable;
    M6Encoder       encoder;
    // --------
    void optimizeMatches(LZMatchParameters *matchTable,
                         size_t *bitCountTable, size_t nBytes);
    void compressData_(const std::vector< unsigned char >& inBuf);
   public:
    Compressor_M6(std::vector< unsigned char >& outBuf_);
    virtual ~Compressor_M6();
    // 'startAddr' must be 0xFFFFFFFF
    // 'isLastBlock' must be true
    // 'enableProgressDisplay' is ignored
    virtual bool compressData(const std::vector< unsigned char >& inBuf,
                              unsigned int startAddr, bool isLastBlock,
                              bool enableProgressDisplay = false);
    void setEncoding(const char *s);
  };

}       // namespace Ep128Compress

#endif  // EPCOMPRESS_COMPRESS6_HPP

