.PHONY: default test
	
.SECONDARY:$(CPP_TEMPS) $(HPP_TEMPS)

# All manner of FLAGS
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


DEBUG=-g -DDEBUG=1

TAMERC=mprpc/tamer/compiler/tamer
TAMERFLAGS=
#TAMERFLAGS=-n -L
ifeq ($(OS),LINUX)
CXX=g++ -std=gnu++0x
endif
ifeq ($(OS),OSX_POSTMAV)
CXX=g++ -std=c++11
endif
CXXFLAGS=-Wall -g -O2 $(DEBUG) -I. -Imprpc -Imprpc/tamer -Imprpc/.deps -include config.h
LIBTAMER=mprpc/tamer/tamer/.libs/libtamer.a
LIBS=$(LIBTAMER) `$(TAMERC) -l`
LDFLAGS= -lpthread -lm $(LIBS)
MPRPC_SRC=mprpc/msgpack.cc mprpc/.deps/mpfd.cc mprpc/string.cc mprpc/straccum.cc mprpc/json.cc mprpc/compiler.cc mprpc/clp.c
MPRPC_OBJ=mprpc/msgpack.o mprpc/mpfd.o mprpc/string.o mprpc/straccum.o mprpc/json.o mprpc/compiler.o mprpc/clp.c
MPRPC_HDR=mprpc/msgpack.hh mprpc/.deps/mpfd.hh mprpc/string.hh mprpc/straccum.hh mprpc/json.hh mprpc/compiler.hh mprpc/clp.h

UTIL_OBJ=network.o

CLIENT_HDR=phat_api.hh puppet.hh rpc_msg.hh log.hh network.hh
SERVER_HDR=phat_server.hh puppet.hh paxos.hh rpc_msg.hh log.hh network.hh

all: main

paxos.o: paxos.cc paxos.hh rpc_msg.hh network.hh $(MPRPC_HDR) $(MPRPC_OBJ) $(MPRPC_SRC)
main.o: main.cc client.hh paxos.o $(UTIL_OBJ) $(MPRPC_OBJ) $(MPRPC_SRC) $(MPRPC_HDR)
main: main.o paxos.o $(UTIL_OBJ) $(MPRPC_OBJ) $(MPRPC_SRC) $(MPRPC_HDR)
	$(CXX) paxos.o $(UTIL_OBJ) $(MPRPC_OBJ) $< -o main $(LDFLAGS)

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
