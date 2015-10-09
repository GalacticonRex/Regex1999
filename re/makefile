PROGNAME = regex1999
PROGDIR = ..

CPP = g++
CPPFLAGS = -std=c++11 -c -MMD -MP -Wall -Wfatal-errors -Os
DLLOPTIONS := -shared -Wl,--out-implib,$(PROGDIR)/$(PROGNAME).a

OBJDIR = obj

SOURCES := $(shell find ./ -name '*.cpp')
SRCDIRS := $(shell dir ./ )
OBJDUMP := $(patsubst %.cpp, %.o, $(SOURCES))
OBJECTS := $(patsubst ./%, $(OBJDIR)/%, $(OBJDUMP))
DEPDUMP := $(patsubst %.o, %.d, $(OBJECTS))

define build_objects
	mkdir -p $(OBJDIR)
endef

debug: DLL_ARG = -D__REGEX_EXPORT__ -D__REGEX_DEBUG_STRING__
debug: build_objdump $(OBJECTS)
	$(CPP) $(OBJECTS) $(LIBS) -o $(PROGDIR)/$(PROGNAME).dll $(DLLOPTIONS)

regex: DLL_ARG = -D__REGEX_EXPORT__
regex: build_objdump $(OBJECTS)
	$(CPP) $(OBJECTS) $(LIBS) -o $(PROGDIR)/$(PROGNAME).dll $(DLLOPTIONS)

$(OBJDIR)/%.o: ./%.cpp
	$(CPP) $(CPPFLAGS) $(DLL_ARG) $< -o $@

build_objdump:
	@$(call build_objects)
	
-include $(DEPDUMP)