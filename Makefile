# S.Chekanov


OPT2          = -O2  
CXX           = g++
CXXFLAGS      = $(OPT2) -Wall -fPIC -std=c++11 
LD            = g++
LDFLAGS       = $(OPT2)
SOFLAGS       = -shared


LIBDIRARCH=lib/
OutPutOpt     = -o  
LIBS         += -L$(PROTOBUF)/lib -lprotobuf
LIBS         += -L$(PROIO)/lib -L$(HOME)/lib -lproio -lproio.pb -lz 
LIBS         += -L$(PYTHIA8)/$(LIBDIRARCH) -lpythia8 -ldl
LIBS	     += -L$(CLHEP)/lib -lCLHEP
LIBS         += -L$(LZ4)/lib -llz4


SOURCE_FILES1 := $(shell ls -1 main.cc)

INCLUDE1=-I./src
INCLUDE2=-I$(LZ4)/include
INCLUDE3=-I$(PROIO)/include 
INCLUDE4=-I$(PROTOBUF)/include
INCLUDE5=-I$(PYTHIA8)/include
INCLUDE6=-I$(CLHEP)/include 

# build object files 
objects1       = $(patsubst %.cc,%.o,$(SOURCE_FILES1))


%.o: %.cc
	$(CXX) $(OPT) $(CXXFLAGS) $(INCLUDE1) $(INCLUDE2) $(INCLUDE3) $(INCLUDE4) $(INCLUDE5) $(INCLUDE6)-o $@ -c $<

Tasks:     clean main.exe


LIBOBJS = $(patsubst %.cc,%.o,$(SOURCE_FILES))

main.exe: $(objects1)
	$(LD) $(LDFLAGS) $^ $(LIBS) $(OutPutOpt)$@

clean:
	        @rm -f *.o *~ main.exe src/*.o ;  echo "Clear.." 
