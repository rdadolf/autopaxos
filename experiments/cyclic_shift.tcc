#include <string>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <cmath>
#include "rpc_msg.hh"
#include "log.hh"
#include "network.hh"
#include "mpfd.hh"
#include "json.hh"
#include "clp.h"

#include "paxos.hh"
#include "client.hh"

using namespace paxos;

int now() {
  struct timeval t;
  gettimeofday(&t,NULL);
  return (int) t.tv_sec;
}

tamed void start_changing_latency(const int delay, const float resolution, const int amplitude) {
  tvars {
    double start;
    double val;
  }
  start = now();
  while (1) {
    twait { at_delay(delay,make_event()); }
    val = sin((now() - start) / (delay / resolution)) * amplitude + amplitude;
    modcomm_fd::set_delay(val);
  }
}

tamed void run() {
  tvars { 
    int n(3),server_port_s(15800),paxos_port_s(15900),master(15900),i;
    std::vector<Paxos_Server*> ps(n);
    Paxos_Client* client;
    Json config = Json::make_array();
    uint64_t data;
    RPC_Msg msg,res; 
  }
  msg = RPC_Msg(Json::object("foo","bar",
                     "blah",Json::array(1,2,3)));
  for (i = 0; i < n; ++i) 
    config.push_back(Json::array(server_port_s + i, paxos_port_s + i));

  for (i = 0; i < n; ++i)
    ps[i] = new Paxos_Server(server_port_s + i, paxos_port_s + i, config,master);
  
  start_changing_latency(1,20.,200);
  twait { at_delay(15,make_event()); }

  tamer::break_loop();
}

int main() {
  // remove persistant files and log
  tamer::initialize();
  system("rm -rf *_persist log.txt");
  run();
  tamer::loop();
  tamer::cleanup();
  return 0;
}
