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
#include "brand.h"

#include "paxos.hh"
#include "client.hh"
#include "telemetry.hh"

using namespace paxos;

// Experimental constants
const int DROP_MIN_INTERVAL = 10;
const int DROP_MAX_INTERVAL = 300;
const int CHANGE_DELAY = 2000;
const int SAMPLE_DELAY = 50;
const int HEARTBEAT_INTERVAL = 150;
const int TIMEOUT_INTERVAL = 500;
const int N_STEPS = 8;
// Experimental state
uint64_t failure_interval = 1000;

tamed void sample_mtbf(const int delay)
{
  tvars { }
  while(1) {
    twait { at_delay_msec(delay, make_event()); }
    DATA() << "MTBF Estimate: " << Telemetry::mtbf_;
  }
}

tamed void fail_node(std::vector<Paxos_Server*> ps, const int i)
{
  tvars { }
  ps[i]->stop();
  DATA() << "Stopped node "<<i;
  twait { at_delay_msec(100, make_event()); }
  ps[i]->start();
  DATA() << "Started node "<<i;
}

tamed void modulate_failure_rate(const int min_interval, const int max_interval)
{
  tvars {
    brand_t br_state;
  }

  brand_init(&br_state, UINT64_C(80858175));
  while(1) {
    failure_interval = ( brand(&br_state) % (max_interval-min_interval) + min_interval );
    DATA() << "Real MTBF set to: " << int64_t(failure_interval);
    twait { at_delay_msec(CHANGE_DELAY, make_event()); }
  }
}


tamed void randomly_fail(std::vector<Paxos_Server*> ps, const int n)
{
  tvars {
    brand_t br_state;
    uint64_t rand_node = 0;
    int nodes_tried = 0;
  }

  brand_init(&br_state, UINT64_C(80858175));
  while(1) {
    rand_node = ( brand(&br_state) % n);
    nodes_tried = 0;
    while(ps[rand_node]->stopped_ || ps[rand_node]->i_am_master() && nodes_tried<n-1) {
      rand_node = (rand_node+1)%n;
      nodes_tried+=1;
    }
    if( nodes_tried<n-1 ) { // If no nodes to kill, skip this cycle
      fail_node(ps,rand_node);
    }
    twait { at_delay_msec(failure_interval, make_event()); }
  }
}

tamed void run() {
  tvars {
    int n(7),server_port_s(15800),paxos_port_s(15900),i;
    std::vector<Paxos_Server*> ps(n);
    Json config = Json::make_array();
    int master(15900);
    int master_index = 0;
  }

  /*
    Experiment: track_drops
    Goal: compare detected to actual drops
    Procedure:
      Modulate failure rate over time.
      Measure estimated MTBF.
  */

  // Server configuration and no-election startup
  for (i = 0; i < n; ++i) 
    config.push_back(Json::array(server_port_s + i, paxos_port_s + i));
  modcomm_fd::set_send_delay(10);
  modcomm_fd::set_recv_delay(0);
  DATA() << "Latency set to: " << 10;
  for (i=0; i<n; ++i) {
    ps[i] = new Paxos_Server(server_port_s+i, paxos_port_s+i, config, master);
    ps[i]->master_timeout_ = 1000000;//never
    ps[i]->heartbeat_freq_ = HEARTBEAT_INTERVAL;
  }
  DATA() << "Heartbeat frequency set to: " << HEARTBEAT_INTERVAL;
  twait { at_delay_msec(1000, make_event()); }

  // Experiment
  modulate_failure_rate(DROP_MIN_INTERVAL, DROP_MAX_INTERVAL);
  randomly_fail(ps, n);
  sample_mtbf(SAMPLE_DELAY);
  twait { at_delay_msec(1000+CHANGE_DELAY*(N_STEPS+1), make_event()); }

  WARN() << "Breaking loop";
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
