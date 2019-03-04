CXXFLAGS  := -Wall -Wextra -Woverloaded-virtual -fPIC -W -pipe -Ofast -O3
ROOTLIBS  := $(shell root-config --libs) -lTreePlayer
ROOTFLAGS := $(shell root-config --cflags)

CXXFLAGS  += $(ROOTFLAGS) $(ROOTLIBS)

OBJFILES := Ranger.o ranger_dict.o
HDRS := LeafBuffer.h Ranger.h

SHARED_LIB := ranger.so

all: ${SHARED_LIB}

%.o: %.cxx
	g++ ${CXXFLAGS} -c $< -o $@

ranger_dict.o: ${HDRS}
	rootcint -f ranger_dict.cc -c ${HDRS}
	g++ $(CXXFLAGS) -c ranger_dict.cc -I. -o ranger_dict.o

${SHARED_LIB}: ${OBJFILES} ${HDRS}
	g++ -shared -O3 ${OBJFILES} -o ${SHARED_LIB}

clean:
	rm -f *.pcm *.cc *o *.so

.PHONY: clean