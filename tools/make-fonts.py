#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Writes launcher/font_data.h from the DejaVu fonts (needs Pillow). The header is kept in the
repository, so building the port needs neither Pillow nor the fonts.
usage: make-fonts.py [folder with DejaVuSans-Bold.ttf and DejaVuSerif-Bold.ttf]"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

FONTS = [('font_title', 'DejaVuSerif-Bold.ttf', 30),
         ('font_item', 'DejaVuSans-Bold.ttf', 18),
         ('font_small', 'DejaVuSans-Bold.ttf', 13)]
where = sys.argv[1] if len(sys.argv) > 1 else '/usr/share/fonts/dejavu'

out = ['/* Made by tools/make-fonts.py from the DejaVu fonts (licenses/dejavu-fonts.txt). Do not edit.',
       ' * One byte of coverage per pixel, ASCII 32 to 126. The glyph data is under the DejaVu fonts\' licence. */',
       'struct glyph { short width, advance, left; int at; };',
       'struct font { int height; const struct glyph *glyphs; const unsigned char *alpha; };']
for name, ttf, size in FONTS:
    font = ImageFont.truetype(os.path.join(where, ttf), size)
    ascent, descent = font.getmetrics()
    height = ascent + descent
    glyphs, data = [], []
    for code in range(32, 127):
        ch = chr(code)
        x0, _, x1, _ = font.getbbox(ch)
        w = max(0, x1 - x0)
        im = Image.new('L', (max(1, w), height), 0)
        ImageDraw.Draw(im).text((-x0, 0), ch, font=font, fill=255)
        glyphs.append((w, int(font.getlength(ch) + 0.5), x0, len(data)))
        if w:
            data.extend(im.tobytes())
    out.append('static const unsigned char %s_alpha[] = {' % name)
    for k in range(0, len(data), 24):
        out.append('\t' + ','.join(str(v) for v in data[k:k + 24]) + ',')
    out.append('};')
    out.append('static const struct glyph %s_glyphs[95] = {' % name)
    out += ['\t{ %d, %d, %d, %d },' % g for g in glyphs]
    out.append('};')
    out.append('static const struct font %s = { %d, %s_glyphs, %s_alpha };' % (name, height, name, name))
path = os.path.join(os.path.dirname(__file__), '..', 'launcher', 'font_data.h')
open(path, 'w').write('\n'.join(out) + '\n')
print(path, os.path.getsize(path), 'bytes')
