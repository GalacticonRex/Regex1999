PROGNAME = regex1999
PROGDIR = ..

COMPILER = gcc
CFLAGS = -c -MMD -MP -Wall -Wfatal-errors -Os
DLLOPTIONS := -shared -Wl,--out-implib,$(PROGDIR)/$(PROGNAME).a

cexe: EXE_ARG = -D__REGEX_EXPORT__
cexe:
	$(COMPILER) $(CFLAGS) $(EXE_ARG) ./regex2.c ./main2.c -o $(PROGNAME).exe
	
cdll: DLL_ARG = -D__REGEX_EXPORT__
cdll:
	$(COMPILER) $(CFLAGS) $(DLL_ARG) ./regex2.c -o $(PROGNAME).dll $(DLLOPTIONS)