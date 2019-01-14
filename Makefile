CC=g++
# common use flags and libraries
CXXFLAGS := -Wall -std=c++17 -Wextra -Woverloaded-virtual -fPIC -W -pipe -Ofast -ftree-vectorize
ROOTLIBS  :=  $(shell root-config --libs) -lRooFit -lRooStats -lTreePlayer
ROOTFLAGS :=  $(shell root-config --cflags)

CXXFLAGS  += $(ROOTFLAGS) $(ROOTLIBS)

LDFLAGS   = -O2 # -Wl
SOFLAGS   = -shared
SHLIB    := lumberjack.so
HDRS     := LeafStore.h LumberJack.h LumberJack_LinkDef.h
COMPILE = $(CC) $(CXXFLAGS) -c

OBJFILES := $(patsubst %.cxx,%.o,$(wildcard *.cxx))
OBJFILES += lumberjack_dict.o

%.o: %.cxx
	$(CC) $(CXXFLAGS) $(DEBUG) -c $< -o $@

$(SHLIB): $(OBJFILES) $(INTS) $(HDRS)
	  /bin/rm -f $(SHLIB)
	  $(CC) $(SOFLAGS) $(LDFLAGS) $(OBJFILES) -o $(SHLIB)

lumberjack_dict.o:  $(HDRS)
	rootcint -f lumberjack_dict.cc -c $(HDRS)
	$(COMPILE) lumberjack_dict.cc -I. -o lumberjack_dict.o

clean:
	rm -f *.pcm *.cc *o *.so
