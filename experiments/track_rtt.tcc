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
#include "brand.h"

#include "paxos.hh"
#include "client.hh"

using namespace paxos;

// Experimental constants
const int MOD_MIN_DELAY = 10;
const int MOD_MAX_DELAY = 100;
const int CHANGE_DELAY = 1000;
const int SAMPLE_DELAY = 100;
const int N_STEPS = 5;


tamed void start_sampling_rtt_estimate(const int delay, Paxos_Server *master)
{
  tvars { }

  while(1) {
    twait {
      at_delay_msec(delay, make_event());
    }
    DATA() << "Master RTT Estimate: " << master->telemetry.rtt_estimate_;
  }
}

tamed void start_changing_latency(const int delay, const int min_value, const int max_value)
{
  tvars {
    brand_t br_state;
    uint64_t rand_delay;
  }

  brand_init(&br_state, UINT64_C(80858175)); // event-loop-proof random numbers

  while(1) {
    twait {
      at_delay_msec(delay, make_event());
    }
    rand_delay = ( brand(&br_state) % (max_value-min_value) ) + min_value;
    modcomm_fd::set_delay(rand_delay);
    DATA() << "Latency set to: " << rand_delay;
  }

}

tamed void run() {
  tvars { 
    int n(3),server_port_s(15800),paxos_port_s(15900),i;
    std::vector<Paxos_Server*> ps(n);
    Json config = Json::make_array();
    uint64_t data;
    int master(15900);    // paxos ports start at 15900
    int master_index = 0; // so master is the first one
    // Experiment variables
    int latency_ms;
    int timeout_value;
  }

  /*
    Plot Goodness vs. Latency
    Goodness (===uptime/traffic)
      uptime: approximate using time for k rounds of iterated paxos
      traffic: measure using # of bytes (or, for now, packets) transferred
    Latency
      Sweep using modcomm
  */

  // Experimental setup
  for (i = 0; i < n; ++i) 
    config.push_back(Json::array(server_port_s + i, paxos_port_s + i));
  modcomm_fd::set_delay(MOD_MIN_DELAY);

  // Synchronous start
  for (i=0; i<n; ++i) {
    ps[i] = new Paxos_Server(server_port_s + i, paxos_port_s + i, config, master);
    ps[i]->master_timeout_ = 1000000;//never
    ps[i]->heartbeat_freq_ = 2*SAMPLE_DELAY;
  }

  // Experiment body
  start_changing_latency(CHANGE_DELAY, MOD_MIN_DELAY, MOD_MAX_DELAY);
  start_sampling_rtt_estimate(SAMPLE_DELAY, ps[master_index]);
  twait {
    at_delay_msec(CHANGE_DELAY*(N_STEPS+1), make_event());
  }

  tamer::break_loop();
}

int main() {
  // remove persistant files and log
  system("rm -rf *_persist log.txt");
  tamer::initialize();
  run();
  tamer::loop();
  tamer::cleanup();
  return 0;
}
