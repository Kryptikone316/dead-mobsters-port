# Fenix - Compilador/intérprete de videojuegos
# Copyright (C) 1999 José Luis Cebrián Pagüe
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# Opciones por defecto
debug = true
target = linux
SDL = /usr

CFLAGS += -Wall -DLINUX

# Usar target=win32 para compilar el .EXE

ifeq ($(target), win32)
    TARGET = ../../bin/fxi.exe
    CFLAGS += -I/win32/include -I$(SDL)/include -I$(SDL)/include/SDL -D_WINDOWS
    LIBS += -lmingw32 -lpng -lz -L$(SDL)/lib -lSDL -mwindows
else
    TARGET = ../../bin/fxi
    CFLAGS += -I$(SDL)/include/SDL -I$(SDL)/include
    LIBS += -lpng -lm -lpthread -L$(SDL)/lib -lSDL -ldl 
endif

# Usar efence=true para enlazar con la librería Electric Fence

ifeq ($(efence), true)
    LIBS += -lefence
endif

# Usar debug=true para mode depurado

ifeq ($(debug), true)
    CFLAGS += -DDEBUG -g
else
    CFLAGS += -O3 -finline-functions -ffast-math 
endif

HEADERS = ../include/fxi.h    ../include/files.h ../include/xctype.h \
	  ../include/offsets.h ../include/pslang.h

CFLAGS += -DTARGET_$(target) -I../include 

MIKMOD_OBJS = \
	../mikmod/load_it.o  ../mikmod/load_m15.o ../mikmod/load_mod.o \
	../mikmod/load_mtm.o ../mikmod/load_s3m.o ../mikmod/load_xm.o \
	../mikmod/mdreg.o    ../mikmod/mdriver.o  ../mikmod/mdulaw.o \
	../mikmod/mloader.o  ../mikmod/mlreg.o    ../mikmod/mlutil.o \
	../mikmod/mmalloc.o  ../mikmod/mmerror.o  ../mikmod/mmio.o \
	../mikmod/mplayer.o  ../mikmod/munitrk.o  ../mikmod/mwav.o \
	../mikmod/npertab.o  ../mikmod/sloader.o  ../mikmod/virtch_common.o \
	../mikmod/virtch2.o  ../mikmod/virtch.o   ../mikmod/drv_sdl.o 

$(TARGET): main.o dcbr.o instance.o strings.o \
	i_main.o i_debug.o i_func.o \
	g_main.o g_maps.o g_blit.o g_texts.o g_pal.o g_draw.o \
	g_systexts.o g_console.o g_scroll.o g_flic.o g_mode7.o \
	img_png.o img_pcx.o sound.o pathfind.o \
	../common/files.o ../common/xctype.o  $(MIKMOD_OBJS)
	cc -rdynamic $(LDEFS) -o $@ $^ $(LIBS)

# Módulo principal

main.o: main.c $(HEADERS)

dcbr.o: dcbr.c $(HEADERS)

# Gestores de datos

instance.o: instance.c $(HEADERS)

strings.o: strings.c $(HEADERS)

# Intérprete

i_main.o: i_main.c $(HEADERS)

i_debug.o: i_debug.c $(HEADERS)

i_func.o: i_func.c $(HEADERS) fmath.h ../include/sysprocs.h

# Rutinas gráficas

g_main.o: g_main.c $(HEADERS)

g_maps.o: g_maps.c $(HEADERS)

g_blit.o: g_blit.c $(HEADERS) fmath.h

g_texts.o: g_texts.c $(HEADERS)

g_systexts.o: g_systexts.c $(HEADERS)

g_console.o: g_console.c $(HEADERS)

g_pal.o: g_pal.c $(HEADERS) font.h

g_scroll.o: g_scroll.c $(HEADERS)

g_flic.o: g_flic.c $(HEADERS)

g_mode7.o: g_mode7.c $(HEADERS) fmath.h

g_draw.o: g_draw.c $(HEADERS)

# Rutinas de carga de imágenes

img_png.o: img_png.c $(HEADERS)

img_pcx.o: img_pcx.c $(HEADERS)

# Módulo de sonido

sound.o: sound.c $(HEADERS)

# Rutinas de utilidad

../common/files.o: ../common/files.c ../include/files.h

../common/xctype.o: ../common/xctype.c ../include/xctype.h

# Limpieza

clean:
	rm -f *.o core ../common/*.o

