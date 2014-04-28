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
MPRPC = $(MPRPC_SRC) $(MPRPC_HDR) $(MPRPC_OBJ)

COMMON_OBJ=network.o paxos.o
COMMON_HDR=log.hh network.hh paxos.hh client.hh
COMMAND_DIR := commands

default: main nnodes $(COMMAND_DIR)/server_command

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Pattern rules for files that need TAMING
%.cc: %.tcc
	$(TAMERC) $(TAMERFLAGS) -o $@ $<
%.hh: %.thh
	$(TAMERC) $(TAMERFLAGS) -o $@ $<

main: main.o paxos.o $(COMMON_OBJ) $(MPRPC_OBJ) $(MPRPC_HDR)
	$(CXX) $(COMMON_OBJ) $(MPRPC_OBJ) $< -o main $(LDFLAGS)

nnodes: nnodes.o paxos.o $(COMMON_OBJ) $(COMMON_HDR) $(MPRPC_OBJ) $(MPRPC_HDR)
	$(CXX) $(COMMON_OBJ) $(MPRPC_OBJ) $< -o nnodes $(LDFLAGS)

EXPERIMENTS=experiments/monotonic_shift
experiments: $(EXPERIMENTS)

$(EXPERIMENTS): %: %.o $(COMMON_OBJ) $(COMMON_HDR) $(MPRPC_OBJ) $(MPRPC_HDR)
	$(CXX) $(COMMON_OBJ) $(MPRPC_OBJ) $< -o $@ $(LDFLAGS)

#other types of behavior here
$(COMMAND_DIR)/server_command: server_command.cc $(COMMON_OBJ) $(COMMON_HDR) $(MPRPC_OBJ) $(MPRPC_HDR)
	$(CXX) $(CXXFLAGS) $(COMMON_OBJ) $(MPRPC_OBJ) $< -o $@ $(LDFLAGS)

network.o: network.hh
paxos.o: paxos.cc paxos.hh
main.o: main.cc paxos.hh client.hh $(COMMON_HDR)
nnodes.o: nnodes.cc paxos.hh $(COMMON_HDR)

TAMED_HH=$(patsubst %.thh,%.hh,$(wildcard *.thh))
TAMED_CC=$(patsubst %.tcc,%.cc,$(wildcard *.tcc))
echo:
	@echo $(TAMED_HH)
	@echo $(TAMED_CC)
# Cleanup
persist_clean:
	rm -f *persist log.txt
	rm -f experiments/*persist experiments/log.txt
clean: persist_clean
	rm -f $(TAMED_HH) $(TAMED_CC)
	rm -f main nnodes *.o
	rm -rf *.dSYM $(COMMAND_DIR)/*.dSYM
	rm -f $(COMMAND_DIR)/{server_command,log.txt}
	rm -f experiments/*.o
	rm -f $(EXPERIMENTS)

always:
	@:

.PHONY: default persist_clean clean always commands experiments default
