# Project: wdump

CPP  = g++.exe
CC   = gcc.exe
WINDRES = windres.exe
RES  = 
OBJ  = obj/wdmp.o obj/wdglb.o obj/dosexe.o obj/os2exe.o obj/peexe.o obj/wdio.o obj/wdwarf.o obj/d16mexe.o obj/qnxexe.o obj/novexe.o obj/pharexe.o obj/elfexe.o obj/dumpcv.o obj/coff.o obj/wdtab.o obj/dumphll.o obj/wdprs.o obj/wdata.o obj/wdseg.o obj/wdres.o obj/wpetbls.o obj/wperes.o obj/dumpwv.o obj/wsect.o obj/wdfix.o obj/typewv.o obj/wline.o
# New for Watcom 1.8
OBJ += obj/machoexe.o
# Files for compiling without Watcom
OBJ += obj/clibext_simp.o
OBJ += $(RES)
LINKOBJ  = $(OBJ)
LIBS =  
INCS =  -I"./watcom/h" -I"./dip/watcom/h" -I"./exedump/h" -I"./compat/h"
CXXINCS =  
BIN  = wdump.exe
CXXFLAGS = $(CXXINCS)  
CFLAGS = -c $(INCS)  
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before wdump.exe all-after


clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LINKOBJ) -o "wdump.exe" $(LIBS)

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
