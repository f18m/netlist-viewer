# =========================================================================
#  NetlistViewer makefile for POSIX targets
#  Needs wx-config utility to be on the PATH.
# =========================================================================



# -------------------------------------------------------------------------
# These are configurable options:
# -------------------------------------------------------------------------

# Location and arguments of wx-config script 
WX_CONFIG ?= wx-config

# C++ compiler 
CXX = `$(WX_CONFIG) --cxx`

# Standard flags for C++
# you can override this to enable e.g. debug mode: -O0 -g
CXXFLAGS ?= -O2


# -------------------------------------------------------------------------
# Do not modify the rest of this file!
# -------------------------------------------------------------------------

# Which Boost modules to use (all)
BOOST_MODULES = \
  graph         \
  serialization

BOOST_MODULES_TYPE := -mt
BOOST_MODULES_LIBS := $(addsuffix $(BOOT_MODULES_TYPE),$(BOOST_MODULES))

BOOST_CXXFLAGS := 
BOOST_LDFLAGS := $(addprefix -lboost_,$(BOOST_MODULES_LIBS))

COMPILER_PREFIX=gcc
WX_CONFIG_FLAGS=
SPICE_VIEWER_CXXFLAGS = -std=c++11 -W -Wall -Isrc `$(WX_CONFIG) --cxxflags $(WX_CONFIG_FLAGS)` $(CXXFLAGS) $(BOOST_CXXFLAGS)
SPICE_VIEWER_LDDFLAGS = $(LDFLAGS) $(BOOST_LDFLAGS) `$(WX_CONFIG) $(WX_CONFIG_FLAGS) --libs adv,core,base`
SPICE_VIEWER_OBJECTS =  \
	$(COMPILER_PREFIX)/spice_viewer_app.o \
	$(COMPILER_PREFIX)/spice_viewer_eng.o \
	$(COMPILER_PREFIX)/spice_viewer_netlist.o


### Targets: ###

all: test_for_selected_wxbuild $(COMPILER_PREFIX) ./NetlistViewer

install: 

uninstall: 

clean: 
	rm -f $(COMPILER_PREFIX)/*.o
	rm -f $(COMPILER_PREFIX)/*.d
	rm -f ./NetlistViewer

test_for_selected_wxbuild:
	@wx-config --list || ( echo "No wx-config utility found on the path. Do you have wxWidgets installed?" ; exit 1 )

$(COMPILER_PREFIX):
	@mkdir -p $(COMPILER_PREFIX)

./NetlistViewer: $(SPICE_VIEWER_OBJECTS)
	$(CXX) -o $@ $(SPICE_VIEWER_OBJECTS)  $(SPICE_VIEWER_LDDFLAGS)

$(COMPILER_PREFIX)/spice_viewer_app.o: ../../src/app.cpp
	$(CXX) -c -o $@ $(SPICE_VIEWER_CXXFLAGS) $(CPPDEPS) $<

$(COMPILER_PREFIX)/spice_viewer_eng.o: ../../src/eng.cpp
	$(CXX) -c -o $@ $(SPICE_VIEWER_CXXFLAGS) $(CPPDEPS) $<

$(COMPILER_PREFIX)/spice_viewer_netlist.o: ../../src/netlist.cpp
	$(CXX) -c -o $@ $(SPICE_VIEWER_CXXFLAGS) $(CPPDEPS) $<

.PHONY: all install uninstall clean


# Dependencies tracking:
-include $(COMPILER_PREFIX)/*.d
