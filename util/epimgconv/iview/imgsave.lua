
fileName = "/tmp/ep_sshot.pic"

 ------------------------------------------------------------------------------

function readVideoMemory(addr)
  return readMemoryRaw(OR(addr, 0x003F0000))
end

function getLPTBase()
  return OR(SHL(readIOPort(0x82), 4), SHL(AND(readIOPort(0x83), 0x0F), 12))
end

function getFixBias()
  return AND(readIOPort(0x80), 0x1F)
end

function getBorderColor()
  return readIOPort(0x81)
end

function readLPT(offs)
  return readVideoMemory(getLPTBase() + offs)
end

function getLPBCount()
  local n = 0
  while AND(readLPB(n, 1), 0x01) == 0 do
    n = n + 1
  end
  return n + 1
end

function readLPB(n, offs)
  return readVideoMemory(getLPTBase() + (n * 16) + offs)
end

function getLPBLines(n)
  return 256 - readLPB(n, 0)
end

function getLPBVideoMode(n)
  return AND(readLPB(n, 1), 0x7E)
end

function getLPBLeftMargin(n)
  return readLPB(n, 2)
end

function getLPBRightMargin(n)
  return readLPB(n, 3)
end

function getLPBLD1Addr(n)
  return OR(readLPB(n, 4), SHL(readLPB(n, 5), 8))
end

function getLPBLD2Addr(n)
  return OR(readLPB(n, 6), SHL(readLPB(n, 7), 8))
end

function getLPBPaletteColor(n, c)
  return readLPB(n, c + 8)
end

function writeEPImageFile(fName)
  local lpbCnt = getLPBCount()
  local fieldCnt = 0
  local leftMargin = -1
  local rightMargin = -1
  local videoMode = -1
  local field0VOffs = false
  local field0Height = -1
  local field0FirstLPB = -1
  local field1VOffs = false
  local field1Height = -1
  local field1FirstLPB = -1
  local paletteSize = 0
  local paletteChangeFlag = false
  local palette = {}
  for i = 0, lpbCnt - 1 do
    if AND(getLPBVideoMode(i), 0x0E) == 0 then
      if AND(getLPBRightMargin(i), 0x3F) > AND(getLPBLeftMargin(i), 0x3F) then
        -- VSYNC
        if field0Height < 0 then
          field0VOffs = (AND(getLPBLeftMargin(i), 0x3F) >= 20)
        elseif field1Height < 0 then
          field1Height = 0
          field1VOffs = (AND(getLPBLeftMargin(i), 0x3F) >= 20)
        else
          field0VOffs = (AND(getLPBLeftMargin(i), 0x3F) >= 20)
        end
        fieldCnt = fieldCnt + 1
        if fieldCnt > 2 then
          return false
        end
      end
    elseif AND(getLPBVideoMode(i), 0x0E) == 0x02 or
            AND(getLPBVideoMode(i), 0x0E) == 0x04 or
            AND(getLPBVideoMode(i), 0x0E) == 0x0E then
      if AND(getLPBRightMargin(i), 0x3F) > AND(getLPBLeftMargin(i), 0x3F) and
         AND(getLPBRightMargin(i), 0x3F) ~= 0x3F then
        if leftMargin < 0 then
          leftMargin = getLPBLeftMargin(i)
          rightMargin = getLPBRightMargin(i)
          videoMode = getLPBVideoMode(i)
          if AND(videoMode, 0x0E) == 0x04 or AND(videoMode, 0x60) == 0x40 then
            paletteSize = 8
          elseif AND(videoMode, 0x60) == 0x60 then
            paletteSize = 0
          else
            paletteSize = SHL(2, SHR(AND(videoMode, 0x60), 5))
          end
        else
          if getLPBLeftMargin(i) ~= leftMargin or
             getLPBRightMargin(i) ~= rightMargin or
             getLPBVideoMode(i) ~= videoMode then
            return false
          end
        end
        if field0FirstLPB < 0 then
          field0FirstLPB = i
          field0Height = getLPBLines(i)
          for j = 0, paletteSize - 1 do
            palette[j] = getLPBPaletteColor(i, j)
          end
        else
          for j = 0, paletteSize - 1 do
            if getLPBPaletteColor(i, j) ~= palette[j] then
              paletteChangeFlag = true
            end
          end
          if field1Height < 0 then
            field0Height = field0Height + getLPBLines(i)
          elseif field1FirstLPB < 0 then
            field1FirstLPB = i
            field1Height = getLPBLines(i)
          else
            field1Height = field1Height + getLPBLines(i)
          end
        end
      end
    end
  end
  if AND(videoMode, 0x10) == 0 then
    return false
  end
  if AND(videoMode, 0x0E) ~= 0x02 and AND(videoMode, 0x0E) ~= 0x04 and
     AND(videoMode, 0x0E) ~= 0x0E then
    return false
  end
  local attributeMode = (AND(videoMode, 0x0E) == 0x04)
  if attributeMode and AND(videoMode, 0x60) ~= 0x00 then
    return false
  end
  if leftMargin > 0x3F or rightMargin > 0x3F then
    return false
  end
  local width = rightMargin - leftMargin
  local height = field0Height
  if fieldCnt < 1 or width < 1 or width > 46 or height < 1 or height > 300 then
    return false
  end
  if field1Height > 0 and field1Height ~= height then
    return false
  end
  if field0VOffs and field1VOffs then
    field0VOffs = false
    field1VOffs = false
  elseif field0VOffs and not field1VOffs then
    field0VOffs = false
    field1VOffs = true
    local tmp = field0FirstLPB
    field0FirstLPB = field1FirstLPB
    field1FirstLPB = tmp
  end
  local f = io.open(fName, "wb")
  if f == nil then
    mprint(" *** error opening output file")
    return false
  end
  -- write header
  f:write(string.char(0x00))
  f:write(string.char(0x49))
  f:write(string.char(0x00))
  f:write(string.char(0x00))
  if paletteChangeFlag then
    f:write(string.char(0x01))
  else
    f:write(string.char(0x00))
  end
  if fieldCnt > 1 then
    local tmp = 0x10
    if paletteChangeFlag then
      tmp = tmp + 0x04
    end
    if attributeMode then
      tmp = tmp + 0x08
    end
    if field1VOffs then
      tmp = tmp + 0x80
    end
    f:write(string.char(tmp))
  else
    f:write(string.char(0x00))
  end
  f:write(string.char(AND(height, 0xFF)))
  f:write(string.char(SHR(height, 8)))
  f:write(string.char(width))
  f:write(string.char(getBorderColor()))
  f:write(string.char(0x00))
  f:write(string.char(fieldCnt))
  f:write(string.char(0x00))
  if attributeMode then
    f:write(string.char(0x01))
  else
    f:write(string.char(0x00))
  end
  f:write(string.char(0x00))
  f:write(string.char(0x00))
  f:write(string.char(videoMode))
  if paletteSize == 8 then
    f:write(string.char(getFixBias()))
  end
  -- write palette
  if paletteSize > 0 and not paletteChangeFlag then
    for j = 0, paletteSize - 1 do
      f:write(string.char(palette[j]))
    end
  elseif paletteSize > 0 then
    i = field0FirstLPB
    j = 0
    while j < field0Height do
      if getLPBVideoMode(i) == videoMode then
        for k = 0, getLPBLines(i) - 1 do
          for l = 0, paletteSize - 1 do
            f:write(string.char(getLPBPaletteColor(i, l)))
          end
          j = j + 1
        end
      end
      i = i + 1
    end
    i = field1FirstLPB
    j = 0
    while j < field1Height do
      if getLPBVideoMode(i) == videoMode then
        for k = 0, getLPBLines(i) - 1 do
          for l = 0, paletteSize - 1 do
            f:write(string.char(getLPBPaletteColor(i, l)))
          end
          j = j + 1
        end
      end
      i = i + 1
    end
  end
  -- write attribute data
  if attributeMode then
    i = field0FirstLPB
    j = 0
    while j < field0Height do
      if getLPBVideoMode(i) == videoMode then
        local baseAddr = getLPBLD1Addr(i)
        for k = 0, getLPBLines(i) - 1 do
          for l = 0, width - 1 do
            local addr = AND(baseAddr + (k * width) + l, 0xFFFF)
            f:write(string.char(readVideoMemory(addr)))
          end
          j = j + 1
        end
      end
      i = i + 1
    end
    i = field1FirstLPB
    j = 0
    while j < field1Height do
      if getLPBVideoMode(i) == videoMode then
        local baseAddr = getLPBLD1Addr(i)
        for k = 0, getLPBLines(i) - 1 do
          for l = 0, width - 1 do
            local addr = AND(baseAddr + (k * width) + l, 0xFFFF)
            f:write(string.char(readVideoMemory(addr)))
          end
          j = j + 1
        end
      end
      i = i + 1
    end
  end
  -- write pixel data
  local bytesPerLine = width
  if AND(videoMode, 0x0E) == 0x02 then
    bytesPerLine = bytesPerLine * 2
  end
  i = field0FirstLPB
  j = 0
  while j < field0Height do
    if getLPBVideoMode(i) == videoMode then
      local baseAddr = getLPBLD1Addr(i)
      if attributeMode then
        baseAddr = getLPBLD2Addr(i)
      end
      for k = 0, getLPBLines(i) - 1 do
        for l = 0, bytesPerLine - 1 do
          local addr = AND(baseAddr + (k * bytesPerLine) + l, 0xFFFF)
          f:write(string.char(readVideoMemory(addr)))
        end
        j = j + 1
      end
    end
    i = i + 1
  end
  i = field1FirstLPB
  j = 0
  while j < field1Height do
    if getLPBVideoMode(i) == videoMode then
      local baseAddr = getLPBLD1Addr(i)
      if attributeMode then
        baseAddr = getLPBLD2Addr(i)
      end
      for k = 0, getLPBLines(i) - 1 do
        for l = 0, bytesPerLine - 1 do
          local addr = AND(baseAddr + (k * bytesPerLine) + l, 0xFFFF)
          f:write(string.char(readVideoMemory(addr)))
        end
        j = j + 1
      end
    end
    i = i + 1
  end
  f:flush()
  f:close()
  return true
end

if not writeEPImageFile(fileName) then
  mprint(" *** error saving screenshot")
else
  mprint("OK")
end

