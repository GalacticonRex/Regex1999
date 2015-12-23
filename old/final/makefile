PROGNAME = regex1999
DEBGNAME = regexdbg
PROGDIR = ..

COMPILER = gcc
CFLAGS = -c -MMD -MP -Wall -Wfatal-errors -Os
DLLOPTIONS = -shared -Wl,--out-implib,lib/$(PROGNAME).a
OPTIMIZE = -O3

cexe: EXE_ARG = -D__REGEX_EXEC__
cexe:
	$(COMPILER) -Wall -Wfatal-errors $(OPTIMIZE) $(EXE_ARG) ./regex.c ./main.c -o $(PROGNAME).exe

cdebug: EXE_ARG = -D__REGEX_EXEC__ -D__DEBUG__
cdebug:
	$(COMPILER) -Wall -Wfatal-errors $(OPTIMIZE) $(EXE_ARG) ./regex.c ./main.c -o $(DEBGNAME).exe
	
cdll: DLL_ARG = -D__REGEX_EXPORT__
cdll:
	$(COMPILER) -Wall -Wfatal-errors $(OPTIMIZE) $(DLL_ARG) ./regex.c -o lib/$(PROGNAME).dll $(DLLOPTIONS)