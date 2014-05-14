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
const int MOD_MAX_DELAY = 200;
const int CHANGE_DELAY = 2000;
const int SAMPLE_DELAY = 50;
const int HEARTBEAT_INTERVAL = 150;
const int N_STEPS = 8;


tamed void sample_rtt_estimate(const int delay)
{
  tvars { }
  while(1) {
    twait { at_delay_msec(delay, make_event()); }
    DATA() << "Master RTT Estimate: " << Telemetry::rtt_estimate_;
  }
}

tamed void modulate_latency(const int delay, const int min_value, const int max_value)
{
  tvars {
    brand_t br_state;
  }
  uint64_t rand_latency; // Temporary; no need to preserve in closure.

  brand_init(&br_state, UINT64_C(80858175)); // event-loop-proof random numbers
  while(1) {
    twait { at_delay_msec(delay, make_event()); }
    rand_latency = ( brand(&br_state) % (max_value-min_value) ) + min_value;
    modcomm_fd::set_send_delay(rand_latency);
    DATA() << "Latency set to: " << rand_latency;
  }

}

tamed void run() {
  tvars { 
    int n(3),server_port_s(15800),paxos_port_s(15900),i;
    std::vector<Paxos_Server*> ps(n);
    Json config = Json::make_array();
    int master(15900);    // paxos ports start at 15900
    int master_index = 0; // so master is the first one
    // Experiment variables
    int latency_ms;
    int timeout_value;
  }

  /*
    Experiment: track_rtt
    Goal: compare speed and accuracy of RTT Estimator vs. Actual Latency
    Procedure:
      Modulate latency over time.
      Measure RTT estimator values.
  */

  // Server configuration and no-election startup
  for (i = 0; i < n; ++i) 
    config.push_back(Json::array(server_port_s + i, paxos_port_s + i));
  modcomm_fd::set_send_delay(MOD_MIN_DELAY);
  modcomm_fd::set_recv_delay(0);
  DATA() << "Latency set to: " << MOD_MIN_DELAY;
  for (i=0; i<n; ++i) {
    ps[i] = new Paxos_Server(server_port_s+i, paxos_port_s+i, config, master);
    ps[i]->master_timeout_ = 1000000;//never
    ps[i]->heartbeat_freq_ = HEARTBEAT_INTERVAL;
  }
  DATA() << "Heartbeat frequency set to: " << HEARTBEAT_INTERVAL;
  twait { at_delay_msec(1000, make_event()); }

  // Experiment
  modulate_latency(CHANGE_DELAY, MOD_MIN_DELAY, MOD_MAX_DELAY);
  sample_rtt_estimate(SAMPLE_DELAY);
  twait { at_delay_msec(1000+CHANGE_DELAY*(N_STEPS+1), make_event()); }

  WARN() << "Breaking loop";
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
