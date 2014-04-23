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
CXXFLAGS=-Wall -g -O2 $(DEBUG) $(DEFINES) -I. -I$(DEPSDIR) -Imprpc -Imprpc/tamer -Imprpc/.deps -I$(DEPSDIR) -include config.h
LIBTAMER=mprpc/tamer/tamer/.libs/libtamer.a
LIBS=$(LIBTAMER) `$(TAMERC) -l`
LDFLAGS= -lpthread -lm $(LIBS)
MPRPC_SRC=mprpc/msgpack.cc mprpc/.deps/mpfd.cc mprpc/string.cc mprpc/straccum.cc mprpc/json.cc mprpc/compiler.cc mprpc/clp.c
MPRPC_OBJ=mprpc/msgpack.o mprpc/mpfd.o mprpc/string.o mprpc/straccum.o mprpc/json.o mprpc/compiler.o mprpc/clp.c
MPRPC_HDR=mprpc/msgpack.hh mprpc/.deps/mpfd.hh mprpc/string.hh mprpc/straccum.hh mprpc/json.hh mprpc/compiler.hh mprpc/clp.h
MPRPC = $(MPRPC_SRC) $(MPRPC_HDR) $(MPRPC_OBJ)

COMMON_OBJ=network.o
COMMON_HDR=log.hh $(DEPSDIR)/network.hh
DEPSDIR := .deps
COMMAND_DIR := commands

default: main nnodes commands

%.o: %.cc $(DEPSDIR)/stamp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: $(DEPSDIR)/%.cc $(DEPSDIR)/stamp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Suffix rules for files that need TAMING
$(DEPSDIR)/%.cc: %.tcc $(DEPSDIR)/stamp
	$(TAMERC) $(TAMERFLAGS) -o $@ $<
$(DEPSDIR)/%.hh: %.thh $(DEPSDIR)/stamp
	$(TAMERC) $(TAMERFLAGS) -o $@ $<

main: main.o paxos.o $(COMMON_OBJ) $(MPRPC_OBJ) $(MPRPC_HDR)
	$(CXX) paxos.o $(COMMON_OBJ) $(MPRPC_OBJ) $< -o main $(LDFLAGS)

# nnodes.o: nnodes.cc paxos.hh $(COMMON_HDR) $(MPRPC_HDR)
nnodes: nnodes.o paxos.o $(COMMON_OBJ) $(COMMON_HDR) $(MPRPC_OBJ) $(MPRPC_HDR)
	$(CXX) paxos.o $(COMMON_OBJ) $(MPRPC_OBJ) $< -o nnodes $(LDFLAGS)

commands: stop_server start_server

%_server: $(DEPSDIR)/server_command.cc $(COMMON_OBJ) $(COMMON_HDR) $(MPRPC_OBJ) $(MPRPC_HDR) $(COMMAND_DIR)/stamp
	$(CXX) $(CXXFLAGS) $(COMMON_OBJ) -DCOMMAND_TYPE_=\"$*\" $(MPRPC_OBJ) $< -o $(COMMAND_DIR)/$@ $(LDFLAGS)

$(COMMAND_DIR)/stamp:
	mkdir -p $(dir $@)
	touch $@

$(DEPSDIR)/stamp:
	mkdir -p $(dir $@)
	touch $@

network.o: $(addprefix $(DEPSDIR)/,network.cc network.hh)
paxos.o: $(addprefix $(DEPSDIR)/,paxos.cc paxos.hh)
main.o: $(addprefix $(DEPSDIR)/,main.cc paxos.hh client.hh) $(COMMON_HDR)
nnodes.o: $(addprefix $(DEPSDIR)/,nnodes.cc paxos.hh) $(COMMON_HDR)

# Cleanup
persist_clean:
	rm -f *_persist log.txt
clean: persist_clean
	rm -f main nnodes *.o
	rm -rf *.dSYM
	rm -rf $(DEPSDIR)
	rm -rf $(COMMAND_DIR)
	rm -f test/*.hh

always:
	@:

.PHONY: default persist_clean clean always
.PRECIOUS: $(DEPSDIR)/%.cc $(DEPSDIR)/%.hh