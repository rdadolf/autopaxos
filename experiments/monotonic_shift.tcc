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
//const int DROP_MIN_INTERVAL = 1000;
//const int DROP_MAX_INTERVAL = 1500;
const int MOD_START_DELAY = 50;
const int MOD_STOP_DELAY = 250;
const int CHANGE_DELAY = 1000;
const int SAMPLE_DELAY = 50;
const int HEARTBEAT_INTERVAL = 280;
const int TIMEOUT_INTERVAL = 1000;
const int EFFICACY_WINDOW = 1000;
const int N_STEPS = 20;
// Experimental state
uint64_t failure_interval = 2000;

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

tamed void sample_traffic(const int delay)
{
  tvars {
    uint64_t last_traffic_count = 0;
  }

  while(1) {
    twait { at_delay_msec(delay, make_event()); }
    DATA() << "Traffic transferred: " << modcomm_fd::data_transferred() - last_traffic_count;
    last_traffic_count = modcomm_fd::data_transferred();
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
    int latency;
  }

  for( latency=min_value; latency<=max_value; latency+=(max_value-min_value)/N_STEPS ) {
    modcomm_fd::set_send_delay(latency);
    // modcomm_fd::set_send_delay( (max_value+min_value)/2 );
    DATA() << "Latency set to: " << latency;
    // DATA() << "Latency set to: " << (max_value+min_value)/2;
    twait { at_delay_msec(delay, make_event()); }
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

#if 0
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
#endif

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
    Experiment: monotonic shift
    Goal: demonstrate auto-tuning of parameters
    Procedure:
      Modulate latency over time.
      Program automatically chooses best parameters for estimated values.
      Measure goodness directly.
  */

  // Server configuration and no-election startup
  for (i = 0; i < n; ++i) 
    config.push_back(Json::array(server_port_s + i, paxos_port_s + i));
  modcomm_fd::set_send_delay(10);
  modcomm_fd::set_recv_delay(0);
  DATA() << "Latency set to: " << 10;
  for (i=0; i<n; ++i) {
    ps[i] = new Paxos_Server(server_port_s+i, paxos_port_s+i, config, master);
    // ps[i]->master_timeout_ = TIMEOUT_INTERVAL;
    ps[i]->master_timeout_ = 1000000; // always wait for master to come back
    ps[i]->heartbeat_interval_ = HEARTBEAT_INTERVAL;
  }
  DATA() << "Heartbeat interval set to: " << HEARTBEAT_INTERVAL;
  twait { at_delay_msec(1000, make_event()); }

  // Experiment
  Paxos_Server::enable_autopaxos = true;

  //modulate_failure_rate(DROP_MIN_INTERVAL, DROP_MAX_INTERVAL);
  failure_interval = 1200;
  DATA() << "Real MTBF set to: " << int64_t(failure_interval);
  randomly_fail(ps, n);
  sample_mtbf(SAMPLE_DELAY);

  modulate_latency(CHANGE_DELAY, MOD_START_DELAY, MOD_STOP_DELAY);
  sample_rtt_estimate(SAMPLE_DELAY);

  //sample_policy(SAMPLE_DELAY);
  sample_traffic(EFFICACY_WINDOW);

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
