# Project: wdump

CPP  = g++
CC   = gcc
WINDRES = windres
RM = rm -f
ECHO = @echo
INCS =  -I"./watcom/h" -I"./dip/watcom/h" -I"./exedump/h" -I"./compat/h" -I"./lib_misc/h"
CXXINCS =
RES  = 
CXXFLAGS = $(CXXINCS)
CFLAGS = -DDEBUG -g -c -Wno-attributes $(INCS)

OBJ_WDUMP  = obj/wdmp.o obj/wdglb.o obj/dosexe.o obj/os2exe.o obj/peexe.o
OBJ_WDUMP += obj/wdio.o obj/wdwarf.o obj/d16mexe.o obj/qnxexe.o obj/novexe.o obj/pharexe.o obj/elfexe.o
OBJ_WDUMP += obj/dumpcv.o obj/coff.o obj/wdtab.o obj/dumphll.o obj/wdprs.o obj/wdata.o obj/wdseg.o obj/wdres.o
OBJ_WDUMP += obj/wpetbls.o obj/wperes.o obj/dumpwv.o obj/wsect.o obj/wdfix.o obj/typewv.o obj/wline.o
# New for Watcom 1.8
OBJ_WDUMP += obj/machoexe.o
# Files for compiling without Watcom
OBJ_WDUMP += obj/clibext_simp.o
OBJ_WDUMP += $(RES)
LIBS_WDUMP =
BIN_WDUMP  = wdump.exe

OBJ_DEMANGLE  = obj/demngl_u.o
OBJ_DEMANGLE += $(RES)
LIBS_DEMANGLE =
BIN_DEMANGLE  = demangle.exe

BIN = $(BIN_WDUMP) $(BIN_DEMANGLE)
OBJ = $(OBJ_WDUMP) $(OBJ_DEMANGLE)

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN_WDUMP): $(OBJ_WDUMP)
	$(CC) $(OBJ_WDUMP) -o $(BIN_WDUMP) $(LIBS_WDUMP)

$(BIN_DEMANGLE): $(OBJ_DEMANGLE)
	$(CC) $(OBJ_DEMANGLE) -o $(BIN_DEMANGLE) $(LIBS_DEMANGLE)

obj/%.o: exedump/c/%.c
	-$(ECHO) 'Building file: $<'
	$(CC) $(CFLAGS) -o"$@" "$<"
	-$(ECHO) 'Finished building: $<'
	-$(ECHO) ' '

obj/%.o: compat/c/%.c
	-$(ECHO) 'Building file: $<'
	$(CC) $(CFLAGS) -o"$@" "$<"
	-$(ECHO) 'Finished building: $<'
	-$(ECHO) ' '

obj/%.o: lib_misc/c/%.c
	-$(ECHO) 'Building file: $<'
	$(CC) $(CFLAGS) -o"$@" "$<"
	-$(ECHO) 'Finished building: $<'
	-$(ECHO) ' '

# Compile demangle with main()
obj/demngl_u.o: lib_misc/c/demangle.c
	-$(ECHO) 'Building file: $<'
	$(CC) $(CFLAGS) -DUTIL -o"$@" "$<"
	-$(ECHO) 'Finished building: $<'
	-$(ECHO) ' '
