#!/usr/bin/env python3
"""Decode TI-89 / TI-92 Plus / V200 program files (.89p, .89g, .9xp, .9xg ...) to readable TI-Basic.

Usage:
    ti89decode.py FILE... [-o OUTPUT.txt] [--pics DIR]

Programs, functions, matrices, lists, strings and expressions are written as text to OUTPUT
(stdout by default); pictures are exported as PNG files into DIR when --pics is given.

Tokenized programs are stored in RPN and read from the end towards the beginning. Token
syntax comes from the GCC4TI documentation of <estack.h> (Tags, ExtTags, InstructionTags).
"""
import argparse
import ctypes
import os
import struct
import sys
import zlib
from decimal import Decimal

EXPR_TYPE, LIST_TYPE, MATRIX_TYPE, STRING_TYPE, PIC_TYPE, PRGM_TYPE, FUNC_TYPE, FOLDER_TYPE = \
    0x00, 0x04, 0x06, 0x0C, 0x10, 0x12, 0x13, 0x1F
END, NEWLINE, NEXTEXPR, ENDSTACK, COMMENT = 0xE5, 0xE8, 0xE7, 0xE9, 0xE6


def _charset():
    """TI-68k charset -> Unicode, via libticonv (installed with TiLP); latin-1 if it is missing."""
    table = [chr(i) for i in range(256)]
    try:
        lib = ctypes.CDLL('libticonv.so.9')
    except OSError:
        return table
    conv = lib.ticonv_ti9x_to_utf16
    conv.restype, conv.argtypes = ctypes.c_void_p, [ctypes.c_char_p, ctypes.c_void_p]
    buf = (ctypes.c_uint16 * 8)()
    for i in range(1, 256):
        conv(bytes([i]), buf)
        table[i] = bytes(buf).decode('utf-16-le').split('\0')[0]
    return table


CHARSET = _charset()


def ti_text(raw):
    return ''.join(CHARSET[c] for c in raw)


# Precedences, low to high
P_STORE, P_WITH, P_OR, P_AND, P_NOT, P_CMP, P_APPEND, P_ADD, P_MUL, P_NEG, P_POW, P_POST, P_ATOM = range(13)

# Binary operators: tag -> (symbol, precedence, left operand is on top of the stack)
BINARY = {
    0x80: ('→', P_STORE, False), 0x81: ('|', P_WITH, True),
    0x82: (' xor ', P_OR, True), 0x83: (' or ', P_OR, True), 0x84: (' and ', P_AND, True),
    0x85: ('<', P_CMP, True), 0x86: ('≤', P_CMP, True), 0x87: ('=', P_CMP, True),
    0x88: ('≥', P_CMP, True), 0x89: ('>', P_CMP, True), 0x8A: ('≠', P_CMP, True),
    0x8B: ('+', P_ADD, False), 0x8C: ('.+', P_ADD, False), 0x8D: ('-', P_ADD, False), 0x8E: ('.-', P_ADD, False),
    0x8F: ('*', P_MUL, False), 0x90: ('.*', P_MUL, False), 0x91: ('/', P_MUL, False), 0x92: ('./', P_MUL, False),
    0x93: ('^', P_POW, True), 0x94: ('.^', P_POW, True), 0xEB: ('±', P_ADD, False),
}
EXT_BINARY = {0x05: ('▶', P_STORE, False), 0x14: ('&', P_APPEND, False), 0x26: ('∠', P_MUL, False)}

PREFIX = {0x79: ('not ', P_NOT), 0x7A: ('-', P_NEG), 0xEA: ('±', P_NEG)}
EXT_PREFIX = {0x01: ('#', P_POST), 0x2B: ('0b', P_ATOM), 0x2C: ('0h', P_ATOM)}

POSTFIX = {0x75: 'ᵀ', 0x76: '!', 0x77: '%', 0x78: 'ʳ', 0xEF: "'"}
EXT_POSTFIX = {0x15: '▶DD', 0x16: '▶DMS', 0x17: '▶Rect', 0x18: '▶Polar', 0x19: '▶Cylind', 0x1A: '▶Sphere',
               0x2D: '▶Bin', 0x2E: '▶Dec', 0x2F: '▶Hex', 0x5F: '▶Grad', 0x60: '▶Rad', 0x61: '▶ln'}

CONSTANTS = {0x24: 'π', 0x25: 'ℯ', 0x26: 'i', 0x27: '-∞', 0x28: '∞', 0x29: '±∞', 0x2A: 'undef',
             0x2B: 'false', 0x2C: 'true', 0x2E: ''}

# Functions written name(args): tag -> (name, fixed argument count, END_TAG-terminated optional args)
FUNCS = {
    0x2F: ('cosh⁻¹', 1, False), 0x30: ('sinh⁻¹', 1, False), 0x31: ('tanh⁻¹', 1, False),
    0x32: ('sech⁻¹', 1, False), 0x33: ('csch⁻¹', 1, False), 0x34: ('coth⁻¹', 1, False),
    0x35: ('cosh', 1, False), 0x36: ('sinh', 1, False), 0x37: ('tanh', 1, False),
    0x38: ('sech', 1, False), 0x39: ('csch', 1, False), 0x3A: ('coth', 1, False),
    0x3B: ('cos⁻¹', 1, False), 0x3C: ('sin⁻¹', 1, False), 0x3D: ('tan⁻¹', 1, False),
    0x3E: ('sec⁻¹', 1, False), 0x3F: ('csc⁻¹', 1, False), 0x40: ('cot⁻¹', 1, False),
    0x41: ('cos⁻¹', 1, False), 0x42: ('sin⁻¹', 1, False), 0x43: ('tan⁻¹', 1, False),
    0x44: ('cos', 1, False), 0x45: ('sin', 1, False), 0x46: ('tan', 1, False),
    0x47: ('sec', 1, False), 0x48: ('csc', 1, False), 0x49: ('cot', 1, False), 0x4A: ('tan', 1, False),
    0x4B: ('abs', 1, False), 0x4C: ('angle', 1, False), 0x4D: ('ceiling', 1, False), 0x4E: ('floor', 1, False),
    0x4F: ('int', 1, False), 0x50: ('sign', 1, False), 0x51: ('√', 1, False), 0x52: ('ℯ^', 1, False),
    0x53: ('ln', 1, False), 0x54: ('log', 1, False), 0x55: ('fPart', 1, False), 0x56: ('iPart', 1, False),
    0x57: ('conj', 1, False), 0x58: ('imag', 1, False), 0x59: ('real', 1, False), 0x5A: ('approx', 1, False),
    0x5B: ('tExpand', 1, False), 0x5C: ('tCollect', 1, False), 0x5D: ('getDenom', 1, False),
    0x5E: ('getNum', 1, False), 0x60: ('cumSum', 1, False), 0x61: ('det', 1, False),
    0x62: ('colNorm', 1, False), 0x63: ('rowNorm', 1, False), 0x64: ('norm', 1, False), 0x65: ('mean', 1, False),
    0x66: ('median', 1, False), 0x67: ('product', 1, False), 0x68: ('stdDev', 1, False), 0x69: ('sum', 1, False),
    0x6A: ('variance', 1, False), 0x6B: ('unitV', 1, False), 0x6C: ('dim', 1, False), 0x6D: ('mat▶list', 1, False),
    0x6E: ('newList', 1, False), 0x6F: ('rref', 1, False), 0x70: ('ref', 1, False), 0x71: ('identity', 1, False),
    0x72: ('diag', 1, False), 0x73: ('colDim', 1, False), 0x74: ('rowDim', 1, False),
    0x7B: ('▶Polar', 1, False), 0x7C: ('▶Cylind', 1, False), 0x7D: ('▶Sphere', 1, False),
    0x95: ('trig', 2, False), 0x96: ('solve', 2, False), 0x97: ('cSolve', 2, False), 0x98: ('nSolve', 2, False),
    0x99: ('zeros', 2, False), 0x9A: ('cZeros', 2, False), 0x9B: ('fMin', 2, False), 0x9C: ('fMax', 2, False),
    0x9E: ('polyEval', 2, False), 0x9F: ('randPoly', 2, False), 0xA0: ('crossP', 2, False), 0xA1: ('dotP', 2, False),
    0xA2: ('gcd', 2, False), 0xA3: ('lcm', 2, False), 0xA4: ('mod', 2, False), 0xA5: ('intDiv', 2, False),
    0xA6: ('remain', 2, False), 0xA7: ('nCr', 2, False), 0xA8: ('nPr', 2, False), 0xA9: ('P▶Rx', 2, False),
    0xAA: ('P▶Ry', 2, False), 0xAB: ('R▶Pθ', 2, False), 0xAC: ('R▶Pr', 2, False), 0xAD: ('augment', 2, False),
    0xAE: ('newMat', 2, False), 0xAF: ('randMat', 2, False), 0xB0: ('simult', 2, False), 0xB1: ('part', 1, True),
    0xB2: ('exp▶list', 2, False), 0xB3: ('randNorm', 2, False), 0xB4: ('mRow', 3, True), 0xB5: ('rowAdd', 3, True),
    0xB6: ('rowSwap', 3, True), 0xB7: ('arcLen', 4, True), 0xB8: ('nInt', 4, True), 0xB9: ('∏', 4, True),
    0xBA: ('Σ', 4, True), 0xBB: ('mRowAdd', 4, True), 0xBC: ('ans', 0, True), 0xBD: ('entry', 0, True),
    0xBE: ('exact', 1, True), 0xBF: ('log', 2, False), 0xC0: ('comDenom', 1, True), 0xC1: ('expand', 1, True),
    0xC2: ('factor', 1, True), 0xC3: ('cFactor', 1, True), 0xC4: ('∫', 2, True), 0xC5: ('d', 2, True),
    0xC6: ('avgRC', 2, True), 0xC7: ('nDeriv', 2, True), 0xC8: ('taylor', 3, True), 0xC9: ('limit', 3, True),
    0xCA: ('propFrac', 1, True), 0xCB: ('when', 2, True), 0xCC: ('round', 1, True), 0xCD: ('dms', 1, True),
    0xCE: ('left', 1, True), 0xCF: ('right', 1, True), 0xD0: ('mid', 2, True), 0xD1: ('shift', 1, True),
    0xD2: ('seq', 4, True), 0xD3: ('list▶mat', 1, True), 0xD4: ('subMat', 1, True), 0xD6: ('rand', 0, True),
    0xD7: ('min', 1, True), 0xD8: ('max', 1, True), 0xED: ('eigVc', 1, False), 0xEE: ('eigVl', 1, False),
    0xF1: ('deSolve', 3, True), 0xF4: ('isPrime', 1, False), 0xF9: ('rotate', 1, True),
}

# E3 xx functions
EXT_FUNCS = {
    0x02: ('getKey', 0, True), 0x03: ('getFold', 0, True), 0x04: ('switch', 0, True), 0x06: ('ord', 1, False),
    0x07: ('expr', 1, False), 0x08: ('char', 1, False), 0x09: ('string', 1, False), 0x0A: ('getType', 1, False),
    0x0B: ('getMode', 1, False), 0x0C: ('setFold', 1, False), 0x0D: ('ptTest', 2, False), 0x0E: ('pxlTest', 2, False),
    0x0F: ('setGraph', 2, False), 0x10: ('setTable', 2, False), 0x11: ('setMode', 0, True), 0x12: ('format', 1, True),
    0x13: ('inString', 2, True), 0x27: ('tmpCnv', 2, False), 0x28: ('ΔtmpCnv', 2, False), 0x29: ('getUnits', 0, True),
    0x2A: ('setUnits', 1, False), 0x30: ('det', 2, False), 0x31: ('ref', 2, False), 0x32: ('rref', 2, False),
    0x33: ('simult', 3, False), 0x34: ('getConfg', 0, True), 0x35: ('augment', 2, False), 0x36: ('mean', 2, False),
    0x37: ('product', 2, True), 0x38: ('stdDev', 2, False), 0x39: ('sum', 2, True), 0x3A: ('variance', 2, False),
    0x3B: ('Δlist', 1, False), 0x46: ('isClkOn', 0, True), 0x47: ('getDate', 0, True), 0x48: ('getTime', 0, True),
    0x49: ('getTmZn', 0, True), 0x4A: ('setDate', 3, True), 0x4B: ('setTime', 3, True), 0x4C: ('setTmZn', 1, False),
    0x4D: ('dayOfWk', 3, True), 0x4E: ('startTmr', 0, True), 0x4F: ('checkTmr', 1, False), 0x50: ('timeCnv', 1, False),
    0x51: ('getDtFmt', 0, True), 0x52: ('getTmFmt', 0, True), 0x53: ('getDtStr', 0, True), 0x54: ('getTmStr', 0, True),
    0x55: ('setDtFmt', 0, True), 0x56: ('setTmFmt', 0, True), 0x57: ('root', 2, False), 0x58: ('exprIO', 1, False),
    0x59: ('impDif', 3, True), 0x5A: ('stDevPop', 1, True), 0x5B: ('isVar', 1, False), 0x5C: ('isLocked', 1, False),
    0x5D: ('isArchiv', 1, False), 0x5E: ('G', 1, False), 0x62: ('▶logbase', 2, False),
}

# E4 xx commands: tag -> (name, fixed argument count, END_TAG-terminated optional args)
COMMANDS = {
    0x01: ('ClrDraw', 0, False), 0x02: ('ClrGraph', 0, False), 0x03: ('ClrHome', 0, False), 0x04: ('ClrIO', 0, False),
    0x05: ('ClrTable', 0, False), 0x06: ('Custom', 0, False), 0x08: ('Dialog', 0, False), 0x09: ('DispG', 0, False),
    0x0A: ('DispTbl', 0, False), 0x0B: ('Else', 0, False), 0x0C: ('EndCustm', 0, False), 0x0D: ('EndDlog', 0, False),
    0x0F: ('EndFunc', 0, False), 0x10: ('EndIf', 0, False), 0x12: ('EndPrgm', 0, False), 0x13: ('EndTBar', 0, False),
    0x14: ('EndTry', 0, False), 0x17: ('Func', 0, False), 0x18: ('Loop', 0, False), 0x19: ('Prgm', 0, False),
    0x1A: ('ShowStat', 0, False), 0x1B: ('Stop', 0, False), 0x1C: ('Then', 0, False), 0x1D: ('Toolbar', 0, False),
    0x1E: ('Trace', 0, False), 0x1F: ('Try', 0, False), 0x20: ('ZoomBox', 0, False), 0x21: ('ZoomData', 0, False),
    0x22: ('ZoomDec', 0, False), 0x23: ('ZoomFit', 0, False), 0x24: ('ZoomIn', 0, False), 0x25: ('ZoomInt', 0, False),
    0x26: ('ZoomOut', 0, False), 0x27: ('ZoomPrev', 0, False), 0x28: ('ZoomRcl', 0, False), 0x29: ('ZoomSqr', 0, False),
    0x2A: ('ZoomStd', 0, False), 0x2B: ('ZoomSto', 0, False), 0x2C: ('ZoomTrig', 0, False), 0x2D: ('DrawFunc', 1, False),
    0x2E: ('DrawInv', 1, False), 0x2F: ('Goto', 1, False), 0x30: ('Lbl', 1, False), 0x31: ('Get', 1, False),
    0x32: ('Send', 1, False), 0x33: ('GetCalc', 1, False), 0x34: ('SendCalc', 1, False), 0x35: ('NewFold', 1, False),
    0x36: ('PrintObj', 1, False), 0x37: ('RclGDB', 1, False), 0x38: ('StoGDB', 1, False), 0x39: ('ElseIf', 1, False),
    0x3A: ('If', 1, False), 0x3B: ('If', 1, False), 0x3C: ('RandSeed', 1, False), 0x3D: ('While', 1, False),
    0x3E: ('LineTan', 2, False), 0x3F: ('CopyVar', 2, False), 0x40: ('Rename', 2, False), 0x41: ('Style', 2, False),
    0x42: ('Fill', 2, False), 0x43: ('Request', 2, False), 0x44: ('PopUp', 2, False), 0x45: ('PtChg', 2, False),
    0x46: ('PtOff', 2, False), 0x47: ('PtOn', 2, False), 0x48: ('PxlChg', 2, False), 0x49: ('PxlOff', 2, False),
    0x4A: ('PxlOn', 2, False), 0x4B: ('MoveVar', 3, False), 0x4C: ('DropDown', 3, False), 0x4D: ('Output', 3, False),
    0x4E: ('PtText', 3, False), 0x4F: ('PxlText', 3, False), 0x50: ('DrawSlp', 3, False), 0x51: ('Pause', 0, True),
    0x52: ('Return', 0, True), 0x53: ('Input', 0, True), 0x54: ('PlotsOff', 0, True), 0x55: ('PlotsOn', 0, True),
    0x56: ('Title', 1, True), 0x57: ('Item', 1, True), 0x58: ('InputStr', 1, True), 0x59: ('LineHorz', 1, True),
    0x5A: ('LineVert', 1, True), 0x5B: ('PxlHorz', 1, True), 0x5C: ('PxlVert', 1, True), 0x5D: ('AndPic', 1, True),
    0x5E: ('RclPic', 1, True), 0x5F: ('RplcPic', 1, True), 0x60: ('XorPic', 1, True), 0x61: ('DrawPol', 0, True),
    0x62: ('Text', 1, True), 0x63: ('OneVar', 0, True), 0x64: ('StoPic', 1, True), 0x65: ('Graph', 1, True),
    0x66: ('Table', 1, True), 0x67: ('NewPic', 2, True), 0x68: ('DrawParm', 2, True), 0x69: ('CyclePic', 2, True),
    0x6A: ('CubicReg', 2, True), 0x6B: ('ExpReg', 2, True), 0x6C: ('LinReg', 2, True), 0x6D: ('LnReg', 2, True),
    0x6E: ('MedMed', 2, True), 0x6F: ('PowerReg', 2, True), 0x70: ('QuadReg', 2, True), 0x71: ('QuartReg', 2, True),
    0x72: ('TwoVar', 2, True), 0x73: ('Shade', 2, True), 0x74: ('For', 3, True), 0x75: ('Circle', 3, True),
    0x76: ('PxlCrcl', 3, True), 0x77: ('NewPlot', 3, True), 0x78: ('Line', 4, True), 0x79: ('PxlLine', 4, True),
    0x7A: ('Disp', 0, True), 0x7B: ('FnOff', 0, True), 0x7C: ('FnOn', 0, True), 0x7D: ('Local', 1, True),
    0x7E: ('DelFold', 1, True), 0x7F: ('DelVar', 1, True), 0x80: ('Lock', 1, True), 0x81: ('Prompt', 1, True),
    0x82: ('SortA', 1, True), 0x83: ('SortD', 1, True), 0x84: ('Unlock', 1, True), 0x85: ('NewData', 2, True),
    0x86: ('Define', 2, False), 0x87: ('Else', 0, False), 0x88: ('ClrErr', 0, False), 0x89: ('PassErr', 0, False),
    0x8A: ('DispHome', 0, False), 0x8B: ('Exec', 1, True), 0x8C: ('Archive', 1, True), 0x8D: ('Unarchiv', 1, True),
    0x8E: ('LU', 4, True), 0x8F: ('QR', 3, True), 0x90: ('BldData', 1, False), 0x91: ('DrwCtour', 1, False),
    0x92: ('NewProb', 0, False), 0x93: ('SinReg', 2, True), 0x94: ('Logistic', 2, True), 0x95: ('CustmOn', 0, False),
    0x96: ('CustmOff', 0, False), 0x97: ('SendChat', 1, False), 0x99: ('Request', 3, True), 0x9A: ('ClockOn', 0, False),
    0x9B: ('ClockOff', 0, False), 0x9C: ('SendCalc', 2, False), 0x9D: ('GetCalc', 2, False), 0x9E: ('DelType', 1, False),
    0x9F: ('Data▶Mat', 2, True), 0xA0: ('Mat▶Data', 2, True),
}
# Loop commands carrying a 2-byte jump displacement below their tag
JUMP_COMMANDS = {0x07: 'Cycle', 0x0E: 'EndFor', 0x11: 'EndLoop', 0x15: 'EndWhile', 0x16: 'Exit'}

SYSVARS = {
    0x01: 'x̄', 0x02: 'ȳ', 0x03: 'Σx', 0x04: 'Σx²', 0x05: 'Σy', 0x06: 'Σy²', 0x07: 'Σxy', 0x08: 'Sx', 0x09: 'Sy',
    0x0A: 'σx', 0x0B: 'σy', 0x0C: 'nStat', 0x0D: 'minX', 0x0E: 'minY', 0x0F: 'q1', 0x10: 'medStat', 0x11: 'q3',
    0x12: 'maxX', 0x13: 'maxY', 0x14: 'corr', 0x15: 'R²', 0x16: 'medx1', 0x17: 'medx2', 0x18: 'medx3',
    0x19: 'medy1', 0x1A: 'medy2', 0x1B: 'medy3', 0x1C: 'xc', 0x1D: 'yc', 0x1E: 'zc', 0x1F: 'tc', 0x20: 'rc',
    0x21: 'θc', 0x22: 'nc', 0x23: 'xfact', 0x24: 'yfact', 0x25: 'zfact', 0x26: 'xmin', 0x27: 'xmax', 0x28: 'xscl',
    0x29: 'ymin', 0x2A: 'ymax', 0x2B: 'yscl', 0x2C: 'Δx', 0x2D: 'Δy', 0x2E: 'xres', 0x2F: 'xgrid', 0x30: 'ygrid',
    0x31: 'zmin', 0x32: 'zmax', 0x33: 'zscl', 0x34: 'eyeθ', 0x35: 'eyeφ', 0x36: 'θmin', 0x37: 'θmax',
    0x38: 'θstep', 0x39: 'tmin', 0x3A: 'tmax', 0x3B: 'tstep', 0x3C: 'nmin', 0x3D: 'nmax', 0x3E: 'plotStrt',
    0x3F: 'plotStep', 0x40: 'zxmin', 0x41: 'zxmax', 0x42: 'zxscl', 0x43: 'zymin', 0x44: 'zymax', 0x45: 'zyscl',
    0x46: 'zxres', 0x47: 'zθmin', 0x48: 'zθmax', 0x49: 'zθstep', 0x4A: 'ztmin', 0x4B: 'ztmax', 0x4C: 'ztstep',
    0x4D: 'zxgrid', 0x4E: 'zygrid', 0x4F: 'zzmin', 0x50: 'zzmax', 0x51: 'zzscl', 0x52: 'zeyeθ', 0x53: 'zeyeφ',
    0x54: 'znmin', 0x55: 'znmax', 0x56: 'zpltstep', 0x57: 'zpltstrt', 0x58: 'seed1', 0x59: 'seed2', 0x5A: 'ok',
    0x5B: 'errornum', 0x5C: 'sysMath', 0x5D: 'sysData', 0x5E: 'regEq', 0x5F: 'regCoef', 0x60: 'tblInput',
    0x61: 'tblStart', 0x62: 'Δtbl', 0x63: 'fldpic', 0x64: 'eyeψ', 0x65: 'tplot', 0x66: 'diftol', 0x67: 'zeyeψ',
    0x68: 't0', 0x69: 'dtime', 0x6A: 'ncurves', 0x6B: 'fldres', 0x6C: 'Estep', 0x6D: 'zt0de', 0x6E: 'ztmaxde',
    0x6F: 'ztstepde', 0x70: 'ztplotde', 0x71: 'ncontour',
}

LETTERS = {0x01: 'q', 0x1B: 'q', **{0x02 + i: c for i, c in enumerate('rstuvwxyz')},
           **{0x0B + i: c for i, c in enumerate('abcdefghijklmnop')}}


class Detokenizer:
    """Reads an RPN token stream backwards, from its last byte (the top of the expression stack)."""

    def __init__(self, data):
        self.b = data
        self.p = len(data) - 1

    def byte(self):
        v = self.b[self.p]
        self.p -= 1
        return v

    def peek(self):
        return self.b[self.p]

    def zstr(self):
        """'\\0' text '\\0', read from the terminating zero."""
        end = self.p
        start = self.b.rindex(0, 0, end)
        self.p = start - 1
        return ti_text(self.b[start + 1:end])

    def number(self):
        n = self.byte()
        self.p -= n
        return int.from_bytes(self.b[self.p + 1:self.p + 1 + n], 'little')

    def float(self):
        raw = self.b[self.p - 8:self.p + 1]
        self.p -= 9
        exp = struct.unpack('>H', raw[:2])[0]
        digits = raw[2:].hex()
        value = Decimal(digits[0] + '.' + digits[1:]).scaleb((exp & 0x7FFF) - 0x4000).normalize()
        text = format(value, 'f') if -6 < value.adjusted() < 12 else format(value, 'E').replace('E+', 'E')
        if '.' not in text and 'E' not in text:
            text += '.'
        return ('-' if exp & 0x8000 else '') + text

    def args(self, fixed, variadic):
        out = [self.expr()[0] for _ in range(fixed)]
        if variadic:
            while self.peek() != END:
                out.append(self.expr()[0])
            self.p -= 1
        return out

    def call(self, spec):
        name, fixed, variadic = spec
        return f"{name}({','.join(self.args(fixed, variadic))})", P_ATOM

    def binary(self, sym, prec, left_on_top):
        a, b = self.expr(), self.expr()
        left, right = (a, b) if left_on_top else (b, a)
        right_assoc = prec == P_POW
        left_t = wrap(left, prec + right_assoc)
        right_t = wrap(right, prec + (not right_assoc))
        return left_t + sym + right_t, prec

    def expr(self):
        """Parse one expression, returns (text, precedence)."""
        t = self.peek()
        if t == 0x00:
            return self.zstr(), P_ATOM
        self.p -= 1
        if t in LETTERS:
            return LETTERS[t], P_ATOM
        if t == 0x1C:
            v = self.byte()
            return SYSVARS.get(v, f'sysvar{v:02X}'), P_ATOM
        if t in (0x1D, 0x1E):
            return ('@' if t == 0x1D else '@n') + str(self.byte()), P_ATOM
        if t in (0x1F, 0x20):
            n = self.number()
            return (str(n), P_ATOM) if t == 0x1F else (f'-{n}', P_NEG)
        if t in (0x21, 0x22):
            num, den = self.number(), self.number()
            return ('' if t == 0x21 else '-') + f'{num}/{den}', P_MUL
        if t == 0x23:
            s = self.float()
            return s, P_NEG if s.startswith('-') else P_ATOM
        if t in CONSTANTS:
            return CONSTANTS[t], P_ATOM
        if t == 0x2D:
            return '"' + self.zstr() + '"', P_ATOM
        if t in FUNCS:
            return self.call(FUNCS[t])
        if t in BINARY:
            return self.binary(*BINARY[t])
        if t in PREFIX:
            sym, prec = PREFIX[t]
            return sym + wrap(self.expr(), prec), prec
        if t in POSTFIX:
            return wrap(self.expr(), P_POST + 1) + POSTFIX[t], P_POST
        if t == 0x9D:
            im, re = self.expr(), self.expr()
            return f'{wrap(re, P_ADD)}+{wrap(im, P_MUL)}*i', P_ADD
        if t == 0xD5:
            var = self.expr()[0]
            return var + '[' + ','.join(self.args(1, True)) + ']', P_ATOM
        if t == 0xD9:
            items = self.args(0, True)
            if items and all(i.startswith('{') for i in items):   # a matrix is a list of lists
                return '[' + ''.join('[' + i[1:-1] + ']' for i in items) + ']', P_ATOM
            return '{' + ','.join(items) + '}', P_ATOM
        if t == 0xDA:
            name = self.expr()[0]
            return f"{name}({','.join(self.args(0, True))})", P_ATOM
        if t == 0xF0:
            return self.expr()
        if t == 0xE3:
            e = self.byte()
            if e in EXT_FUNCS:
                return self.call(EXT_FUNCS[e])
            if e in EXT_BINARY:
                return self.binary(*EXT_BINARY[e])
            if e in EXT_PREFIX:
                sym, prec = EXT_PREFIX[e]
                return sym + wrap(self.expr(), P_POST), prec
            if e in EXT_POSTFIX:
                return wrap(self.expr(), P_POST + 1) + EXT_POSTFIX[e], P_POST
            raise ValueError(f'unknown extended tag E3 {e:02X} at offset {self.p + 1}')
        raise ValueError(f'unknown tag {t:02X} at offset {self.p + 1}')

    def command(self):
        c = self.byte()
        if c in JUMP_COMMANDS:
            self.p -= 2
            return JUMP_COMMANDS[c]
        if c not in COMMANDS:
            raise ValueError(f'unknown command E4 {c:02X} at offset {self.p + 1}')
        name, fixed, variadic = COMMANDS[c]
        args = self.args(fixed, variadic)
        if c == 0x3B:
            return f'If {args[0]} Then'
        if c == 0x86:
            return f'Define {args[0]}={args[1]}'
        return name + (' ' + ','.join(args) if args else '')

    def statement(self):
        t = self.peek()
        if t in (NEWLINE, NEXTEXPR, ENDSTACK):   # empty line
            return ''
        if t == 0xE4:
            self.p -= 1
            return self.command()
        if t == COMMENT:
            self.p -= 1
            spaces = self.byte()
            return '©' + ' ' * spaces + self.zstr()
        return self.expr()[0]

    def program(self):
        """Tokenized program/function body, returns its lines."""
        self.p -= 4                      # USER_DEF_TAG, flags, 2 reserved bytes
        params = self.args(0, True)
        lines, line, indent = [], [], 0
        while True:
            line.append(self.statement())
            sep = self.peek()
            if sep == NEXTEXPR:
                self.p -= 2
                continue
            lines.append(' ' * indent + ':'.join(line))
            line = []
            if sep == ENDSTACK:
                break
            if sep != NEWLINE:
                raise ValueError(f'unexpected tag {sep:02X} at offset {self.p}')
            self.p -= 1
            indent = self.byte()
        return ['(' + ','.join(params) + ')'] + lines


def wrap(item, min_prec):
    text, prec = item
    return f'({text})' if prec < min_prec else text


def decode_program(body):
    if body[-2] & 0x08:                  # never run since edited: still stored as plain text
        return ti_text(body[:body.index(0)]).split('\r')
    return Detokenizer(body).program()


def decode_matrix(body):
    """One line per matrix row."""
    d = Detokenizer(body)
    if d.byte() != 0xD9:
        raise ValueError('not a matrix')
    rows = []
    while d.peek() != END:
        rows.append('[' + d.expr()[0][1:-1] + ']')
    return rows


def decode_expr(body):
    return [Detokenizer(body).expr()[0]]


def write_png(path, body):
    """PIC variable: rows, columns (big endian), then 1 bit per pixel, rows padded to a byte."""
    rows, cols = struct.unpack_from('>HH', body)
    stride = (cols + 7) // 8
    bits = body[4:4 + rows * stride]
    raw = b''.join(b'\0' + bytes(~c & 0xFF for c in bits[r * stride:(r + 1) * stride]) for r in range(rows))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', cols, rows, 1, 0, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


DECODERS = {PRGM_TYPE: ('programme', decode_program), FUNC_TYPE: ('fonction', decode_program),
            MATRIX_TYPE: ('matrice', decode_matrix), LIST_TYPE: ('liste', decode_expr),
            STRING_TYPE: ('chaîne', decode_expr), EXPR_TYPE: ('expression', decode_expr)}


def read_vars(path):
    """Yield (folder, name, type, body) for each variable of a TI-89/92+ single or group file."""
    d = open(path, 'rb').read()
    if not d.startswith(b'**TI92P*') and not d.startswith(b'**TI89**'):
        sys.exit(f'{path}: not a TI-89/TI-92 Plus file')
    folder = d[0x0A:0x12].split(b'\0')[0].decode('latin-1')
    count = struct.unpack_from('<H', d, 0x3A)[0]
    for i in range(count):
        entry = 0x3C + 16 * i
        offset = struct.unpack_from('<I', d, entry)[0]
        name = d[entry + 4:entry + 12].split(b'\0')[0].decode('latin-1')
        vtype = d[entry + 12]
        if vtype == FOLDER_TYPE:
            folder = name
            continue
        size = struct.unpack_from('>H', d, offset + 4)[0]
        yield folder, name, vtype, d[offset + 6:offset + 6 + size]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('files', nargs='+')
    ap.add_argument('-o', '--output', help='write everything into this text file (default: stdout)')
    ap.add_argument('--pics', help='export PIC variables as PNG files into this directory')
    args = ap.parse_args()
    out = open(args.output, 'w', encoding='utf-8') if args.output else sys.stdout
    if args.pics:
        os.makedirs(args.pics, exist_ok=True)
    for path in args.files:
        for folder, name, vtype, body in read_vars(path):
            if vtype == PIC_TYPE:
                if args.pics:
                    write_png(os.path.join(args.pics, name + '.png'), body)
                continue
            if vtype not in DECODERS:
                print(f'# {folder}\\{name}: skipped (variable type 0x{vtype:02X})', file=sys.stderr)
                continue
            kind, decode = DECODERS[vtype]
            try:
                lines = decode(body)
            except (ValueError, IndexError) as e:
                print(f'# {folder}\\{name}: decode error: {e}', file=sys.stderr)
                continue
            if vtype in (PRGM_TYPE, FUNC_TYPE):
                lines[0] = name + lines[0]
            print(f'==================== {os.path.basename(path)} : {folder}\\{name} ({kind}) ====================',
                  file=out)
            print('\n'.join(lines) + '\n', file=out)
    if args.output:
        out.close()


if __name__ == '__main__':
    main()
