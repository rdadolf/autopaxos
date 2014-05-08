#include <string>
#include <stdlib.h>
#include <assert.h>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include "rpc_msg.hh"
#include "log.hh"
#include "network.hh"
#include "mpfd.hh"
#include "json.hh"
#include "clp.h"

#include "paxos.hh"
#include "client.hh"

#define MODCOMM_DELAY 0

using namespace paxos;

tamed void make_requests(Paxos_Client* client,tamer::event<> ev) {
  tvars {
    tamer::destroy_guard guard(client);
    RPC_Msg msg,res; 
  }
  msg = RPC_Msg(Json::object("foo","bar",
                     "blah",Json::array(1,2,3)));
  // spin and make events
  while (1)
    twait { client->make_request(msg,make_event(res.json())); }
  
  ev();
}

tamed void run() {
  tvars { 
    int n(3),server_port_s(15800),paxos_port_s(15900),master(15900),i,j;
    int iterations(3), window(2);
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
  
  for (j = 0; j < iterations; j++) {
    system("rm -rf *_persist");
    INFO() << "window " << j << " starting.";
    for (i = 0; i < n; ++i)
      ps[i] = new Paxos_Server(server_port_s + i, paxos_port_s + i, config,master);
    
    modcomm_fd::set_delay(0); // FIXME

    // initialize client
    client = new Paxos_Client(config);
    twait { client->get_master(make_event()); }

    // make requests here
    twait { make_requests(client,with_timeout(window, make_event())); }
    // twait { client->make_request(msg,make_event(res.json())); }

    // twait { tamer::at_delay(window,make_event()); }

    INFO() << "window " << j << " done.";

    data = modcomm_fd::data_transferred();
    DATA() << "Session stats: " << data << " packets sent over 5 seconds.";

    for (i = 0; i < n; ++i)
      delete ps[i];
    delete client;
  }
  // FIXME: trigger paxos sync start event

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
