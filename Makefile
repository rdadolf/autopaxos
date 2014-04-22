.PHONY: default test

CPP_TEMPS= main.cc paxos.cc network.cc
HPP_TEMPS= paxos.hh network.hh
.SECONDARY:$(CPP_TEMPS) $(HPP_TEMPS)

default: main

# All manner of FLAGS

### OS Detection
OS_STYLE=$(shell uname -s)
OS_MAJOR=$(shell uname -r | sed -e 's/\..*//')
OS_MINOR=$(shell uname -r | sed -e 's/[^.]*\.\([^.]*\).*/\1/')
OS_VERSION=$(shell uname -r | perl -ne '/(\d+\.\d+)/; print 0+$$1')
OS=UNKNOWN
ifeq ($(OS_STYLE), Linux)
OS=LINUX
endif
ifeq ($(OS_STYLE),Darwin)
ifeq ($(shell perl -e 'print int($(OS_MAJOR)>=13 && $(OS_MINOR)>=0)'), 1)
OS=OSX_POSTMAV
else
OS=OSX_PREMAV
endif
endif

### Compile switches
# comment/uncomment
#DEBUG=-g -DDEBUG=1
DEFINES=-DMODCOMM_DELAY=0 -DMASTER_TIMEOUT=5000 -DHEARTBEAT_FREQ=3500

TAMERC=mprpc/tamer/compiler/tamer
TAMERFLAGS=
#TAMERFLAGS=-n -L
ifeq ($(OS),LINUX)
CXX=g++ -std=gnu++0x
endif
ifeq ($(OS),OSX_POSTMAV)
CXX=g++ -std=c++11
endif
CXXFLAGS=-Wall -g -O2 $(DEBUG) $(DEFINES) -I. -Imprpc -Imprpc/tamer -Imprpc/.deps -include config.h
LIBTAMER=mprpc/tamer/tamer/.libs/libtamer.a
LIBS=$(LIBTAMER) `$(TAMERC) -l`
LDFLAGS= -lpthread -lm $(LIBS)
MPRPC_SRC=mprpc/msgpack.cc mprpc/.deps/mpfd.cc mprpc/string.cc mprpc/straccum.cc mprpc/json.cc mprpc/compiler.cc mprpc/clp.c
MPRPC_OBJ=mprpc/msgpack.o mprpc/mpfd.o mprpc/string.o mprpc/straccum.o mprpc/json.o mprpc/compiler.o mprpc/clp.c
MPRPC_HDR=mprpc/msgpack.hh mprpc/.deps/mpfd.hh mprpc/string.hh mprpc/straccum.hh mprpc/json.hh mprpc/compiler.hh mprpc/clp.h

COMMON_OBJ=network.o
COMMON_HDR=log.hh network.hh

paxos.o: paxos.cc paxos.hh rpc_msg.hh $(COMMON_HDR) $(MPRPC_HDR)
main.o: main.cc paxos.hh client.hh $(COMMON_HDR) $(MPRPC_HDR)
main: main.o paxos.o $(COMMON_OBJ) $(MPRPC_OBJ) $(MPRPC_HDR)
	$(CXX) paxos.o $(COMMON_OBJ) $(MPRPC_OBJ) $< -o main $(LDFLAGS)

# Suffix rules for files that need TAMING
%.cc: %.tcc
	$(TAMERC) $(TAMERFLAGS) -o $@ $<
%.cc: test/%.tcc
	$(TAMERC) $(TAMERFLAGS) -o $@ $<
%.hh: %.thh
	$(TAMERC) $(TAMERFLAGS) -o $@ $<

# Cleanup
clean:
	rm -f main *.o *_persist log.txt
	rm -rf *.dSYM
	rm -f $(CPP_TEMPS)
	rm -f $(HPP_TEMPS)
	rm -f test/*.hh
