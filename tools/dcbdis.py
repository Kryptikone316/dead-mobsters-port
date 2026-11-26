#!/usr/bin/env python3
# SPDX-License-Identifier: Unlicense
"""Small disassembler for Fenix 0.80 bytecode, for working out what a game does.
usage: dcbdis.py <fenix080 dir> <game.dcb>"""
import re, struct, sys
fx, path = sys.argv[1], sys.argv[2]
read = lambda p: open(p, encoding='latin-1').read()
ops = {}
for m in re.finditer(r'#define\s+(MN_\w+)\s+\((0x[0-9A-F]+)\s*\|\s*MN_(\d)_PARAMS\)', read(fx + '/include/pslang.h')):
    ops[int(m[2], 16) | (0x80 if m[3] == '1' else 0)] = m[1][3:]
sysp = {int(m[1], 16): (m[2], int(m[3])) for m in re.finditer(
    r'\{\s*(0x[0-9A-Fa-f]+),\s*"([^"]+)"\s*,\s*"[^"]*"\s*,\s*\w+\s*,\s*(\d+)', read(fx + '/include/sysprocs.h'))}
d = open(path, 'rb').read()
names = ('NProcs NFiles NID NStrings NLocStrings NLocVars NGloVars NVarSpaces SGlobal SLocal SText NImports NSourceFiles r1 r2 r3 '
         'OProcsTab OID OStrings OText OGlobal OGloVars OLocal OLocVars OLocStrings OVarSpaces OFilesTab OImports OSourceFiles').split()
h = dict(zip(names, struct.unpack_from('<%dI' % len(names), d, 12)))
ids = {}
for i in range(h['NID']):
    n, c = struct.unpack_from('<60sI', d, h['OID'] + i * 64)
    ids[c] = n.split(b'\0')[0].decode('latin-1')
off = struct.unpack_from('<%dI' % h['NStrings'], d, h['OStrings'])
text = d[h['OText']:h['OText'] + h['SText']]
strs = [text[o:text.index(b'\0', o)].decode('latin-1') for o in off]
print({k: h[k] for k in names[:13]})
print('IDS', ids)
print('STRINGS', list(enumerate(strs)))
for i in range(h['NProcs']):
    pid, npar, npv, nps, nsen, spriv, scode, opriv, opv, ops_, ocode, osen = struct.unpack_from('<12I', d, h['OProcsTab'] + i * 48)
    code = struct.unpack_from('<%di' % (scode // 4), d, ocode)
    print('\n== proc %d %s params=%d' % (i, ids.get(pid, ''), npar))
    k = 0
    while k < len(code):
        raw = code[k]; base = raw & 0xff; at = k; k += 1
        nm = ops.get(base, 'op%02x' % base)
        suf = {0: '', 1: '.w', 2: '.b', 3: '.s', 4: '.f'}.get(raw >> 8, '.?')
        arg = ''
        if base & 0x80:
            v = code[k]; k += 1; arg = str(v)
            if nm in ('SYSCALL', 'SYSPROC'): arg = sysp.get(v, ('?%x' % v, 0))[0]
            elif nm in ('CALL', 'PROC', 'TYPE'): arg = 'proc%d' % v
        print('%5d %s%s %s' % (at, nm, suf, arg))
