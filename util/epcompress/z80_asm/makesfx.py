#!/usr/bin/python

import os

sjasmCmd = "sjasm"
tmpSrcFileName = "tmp.s"
tmpLstFileName = "tmp.lst"
tmpOutFileName = "tmp.bin"

def compileFile(outputString, fileName, variableList):
    inFile = open(fileName, "rb")
    outFile = open(tmpSrcFileName, "wb")
    while True:
        s = inFile.readline()
        if s == '':
            break
        if '\r' in s:
            s = s.replace('\r', '')
        if '\n' in s:
            s = s.replace('\n', '')
        nVars = variableList.__len__() >> 1
        for i in range(nVars):
            vName = variableList[i << 1]
            vValue = variableList[(i << 1) + 1]
            sTmp = s.strip()
            if sTmp.__len__() > vName.__len__() and sTmp.find(vName) == 0:
                c = sTmp[vName.__len__()]
                if c == ' ' or c == '\t':
                    s = vName + "    equ    " + vValue
                    break
        s += "\r\n"
        outFile.write(s)
    outFile.flush()
    outFile.close()
    inFile.close()
    os.spawnvp(os.P_WAIT, sjasmCmd, [sjasmCmd, tmpSrcFileName, tmpOutFileName])
    try:
        os.remove(tmpSrcFileName)
    except:
        pass
    try:
        os.remove(tmpLstFileName)
    except:
        pass
    inFile = open(tmpOutFileName, "rb")
    n = 0
    outputString += " "
    while True:
        c = inFile.read(1)
        if c == '':
            break
        if n > 0:
            outputString += ","
            if (n % 12) == 0:
                outputString += "\n "
        outputString += (" 0x%02X" % ord(c))
        n = n + 1
    inFile.close()
    try:
        os.remove(tmpOutFileName)
    except:
        pass
    outputString += "\n"
    return outputString

sfxCode = "\n"
sfxCode += "// compressionType = 3, noBorderFX = false, noCleanup = false\n"
sfxCode += "\n"
sfxCode += "static const unsigned char epSFXModule_M3_0[] = {\n"
sfxCode = compileFile(sfxCode, "decompress_sfx_m3.s",
                      ["NO_BORDER_FX", "0",
                       "NO_CLEANUP", "0"])
sfxCode += "};\n"
sfxCode += "\n"
sfxCode += "// compressionType = 3, noBorderFX = true, noCleanup = false\n"
sfxCode += "\n"
sfxCode += "static const unsigned char epSFXModule_M3_1[] = {\n"
sfxCode = compileFile(sfxCode, "decompress_sfx_m3.s",
                      ["NO_BORDER_FX", "1",
                       "NO_CLEANUP", "0"])
sfxCode += "};\n"
sfxCode += "\n"
sfxCode += "// compressionType = 3, noBorderFX = false, noCleanup = true\n"
sfxCode += "\n"
sfxCode += "static const unsigned char epSFXModule_M3_2[] = {\n"
sfxCode = compileFile(sfxCode, "decompress_sfx_m3.s",
                      ["NO_BORDER_FX", "0",
                       "NO_CLEANUP", "1"])
sfxCode += "};\n"
sfxCode += "\n"
sfxCode += "// compressionType = 3, noBorderFX = true, noCleanup = true\n"
sfxCode += "\n"
sfxCode += "static const unsigned char epSFXModule_M3_3[] = {\n"
sfxCode = compileFile(sfxCode, "decompress_sfx_m3.s",
                      ["NO_BORDER_FX", "1",
                       "NO_CLEANUP", "1"])
sfxCode += "};\n"
sfxCode += "\n"

sfxCode += "// self-extracting decompressor code for .ext files (compressionType = 3)\n"
sfxCode += "\n"
sfxCode += "static const unsigned char epEXTSFXModule_M3[] = {\n"
sfxCode = compileFile(sfxCode, "decompress_ext_m3.s", [])
sfxCode += "};\n"
sfxCode += "\n"

outFile = open("sfxcode.cpp", "w")
outFile.write(sfxCode)
outFile.flush()
outFile.close()

