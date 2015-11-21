PROGNAME = regex1999
PROGDIR = ..

COMPILER = gcc
CFLAGS = -c -MMD -MP -Wall -Wfatal-errors -Os
DLLOPTIONS := -shared -Wl,--out-implib,$(PROGDIR)/$(PROGNAME).a

cexe:
	$(COMPILER) -Wall -Wfatal-errors -Os ./regex.c ./main.c -o regex.exe
	
cversion: DLL_ARG = -D__REGEX_EXPORT__
cversion:
	$(COMPILER) $(CFLAGS) $(DLL_ARG) ./regex.c -o $(PROGDIR)/$(PROGNAME).dll $(DLLOPTIONS)