#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Writes shim/gamma_tables.h. The tables are made here so that the handheld never has to call pow()."""
import os
GAMMAS = range(6, 17)    # tenths
out = ['/* SPDX-License-Identifier: MIT */',
       '/* Made by tools/make-gamma.py. Do not edit. out = 255 * (in / 255) ^ (1 / gamma), gamma 0.6 to 1.6. */',
       'static const unsigned char dm_gamma_tables[%d][256] = {' % len(GAMMAS)]
for g in GAMMAS:
    vals = [int(255 * (i / 255) ** (10 / g) + 0.5) for i in range(256)]
    out.append('\t{ /* %.1f */' % (g / 10))
    for k in range(0, 256, 16):
        out.append('\t\t' + ','.join('%3d' % v for v in vals[k:k + 16]) + ',')
    out.append('\t},')
out.append('};')
path = os.path.join(os.path.dirname(__file__), '..', 'shim', 'gamma_tables.h')
open(path, 'w').write('\n'.join(out) + '\n')
