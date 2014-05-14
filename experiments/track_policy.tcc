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
#include "policy.hh"

using namespace paxos;

// Experimental constants
const int DROP_MIN_INTERVAL = 1000;
const int DROP_MAX_INTERVAL = 1500;
const int MOD_MIN_DELAY = 100;
const int MOD_MAX_DELAY = 200;
const int CHANGE_DELAY = 20000;
const int SAMPLE_DELAY = 50;
const int HEARTBEAT_INTERVAL = 280;
const int TIMEOUT_INTERVAL = 880;
const int N_STEPS = 10;
// Experimental state
uint64_t failure_interval = 1000;

tamed void sample_rtt_estimate(const int delay)
{
  tvars { }
  while(1) {
    twait { at_delay_msec(delay, make_event()); }
    DATA() << "Master RTT Estimate: " << Telemetry::rtt_estimate_;
  }
}

tamed void sample_mtbf(const int delay)
{
  tvars { }
  while(1) {
    twait { at_delay_msec(delay, make_event()); }
    DATA() << "MTBF Estimate: " << Telemetry::mtbf_;
  }
}

tamed void sample_policy(const int delay)
{
  tvars { 
    double T_hb = double(HEARTBEAT_INTERVAL);
    double T_bf;      // MTBF estimate, in us
    double T_to = double(TIMEOUT_INTERVAL);
    double T_l;       // Latency estimate, in us
    double C_r = 20.; // Recovery cost, in packets
    double C_hb = 2.; // Heartbeat cost, in packets
  }
  while(1) {
    twait { at_delay_msec(delay, make_event()); }
    T_bf = double(Telemetry::mtbf_)/1000.;
    //T_bf = failure_interval; //FIXME: Cheating
    T_l = double(Telemetry::rtt_estimate_)/2000.;
    //DATA() << "PMetric Estimate: pmetric("<<T_hb<<", "<<T_bf<<", "<<T_to<<", "<<T_l<<", "<<C_r<<", "<<C_hb<<")";
    DATA() << "PMetric Estimate: " << pmetric(T_hb, T_bf, T_to, T_l, C_r, C_hb);
  }
}

////////////////////////////////////////////////////////////////////////////////

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


tamed void fail_node(std::vector<Paxos_Server*> ps, const int i)
{
  tvars { }
  ps[i]->stop();
  DATA() << "Stopped node "<<i;
  twait { at_delay_msec(TIMEOUT_INTERVAL+100, make_event()); }
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
    //while((ps[rand_node]->stopped_ || ps[rand_node]->i_am_master()) && nodes_tried<n-1) {
    while(ps[rand_node]->stopped_ && nodes_tried<n-1) {
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
    // Experiment variables
    int latency_ms;
    int timeout_value;
  }

  /*
    Experiment: track_policy
    Goal: compare estimated policy to actual values
    Procedure:
      Modulate failure rate over time.
      Modulate latency over time.
      Measure estimated RTT values.
      Measure estimated MTBF.
      Compute goodness using known values.
      Choose best parameters for estimated values.
      Estimate goodness.
  */

  // Server configuration and no-election startup
  for (i = 0; i < n; ++i) 
    config.push_back(Json::array(server_port_s + i, paxos_port_s + i));
  modcomm_fd::set_send_delay(10);
  modcomm_fd::set_recv_delay(0);
  DATA() << "Latency set to: " << 10;
  for (i=0; i<n; ++i) {
    ps[i] = new Paxos_Server(server_port_s+i, paxos_port_s+i, config, master);
    //ps[i]->master_timeout_ = TIMEOUT_INTERVAL;
    ps[i]->master_timeout_ = 1000000; // always wait for the master to come back
    ps[i]->heartbeat_interval_ = HEARTBEAT_INTERVAL;
  }
  DATA() << "Heartbeat interval set to: " << HEARTBEAT_INTERVAL;
  twait { at_delay_msec(1000, make_event()); }

  // Experiment
  modulate_failure_rate(DROP_MIN_INTERVAL, DROP_MAX_INTERVAL);
  randomly_fail(ps, n);
  sample_mtbf(SAMPLE_DELAY);

  modulate_latency(CHANGE_DELAY, MOD_MIN_DELAY, MOD_MAX_DELAY);
  sample_rtt_estimate(SAMPLE_DELAY);

  sample_policy(SAMPLE_DELAY);

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
