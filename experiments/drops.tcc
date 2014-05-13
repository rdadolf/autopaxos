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

tamed void start_changing_latency(const int delay, const int min_value, const int max_value)
{
  tvars {
    brand_t br_state;
    uint64_t rand_latency;
  }

  brand_init(&br_state, UINT64_C(80858175)); // event-loop-proof random numbers

  while(1) {
    twait {
      at_delay_msec(delay, make_event());
    }
    rand_latency = ( brand(&br_state) % (max_value-min_value) ) + min_value;
    modcomm_fd::set_delay(rand_latency);
    DATA() << "Latency set to: " << rand_latency;
  }

}

tamed void run(const int delay, const int min_freq, const int max_freq) {
    tvars { 
        int n(3),server_port_s(15800),paxos_port_s(15900),master(15900),i,j;
        std::vector<Paxos_Server*> ps(n);
        Paxos_Client* client;
        Json config = Json::make_array();
        uint64_t data;
        RPC_Msg msg,res; 
        Telemetry telemetry_;
        brand_t br_state;
        uint64_t rand_ind;
        uint64_t rand_hbf;
    }

    brand_init(&br_state, UINT64_C(80858175)); // event-loop-proof random numbers

    for (i = 0; i < n; ++i) 
        config.push_back(Json::array(server_port_s + i, paxos_port_s + i));

    for (i = 0; i < n; ++i)
        ps[i] = new Paxos_Server(server_port_s + i, paxos_port_s + i, config,master);

    // tests for not stopping the master
    for (i = 0; i < 7 ; ++i) {
        rand_ind = brand(&br_state) % n; // randomly choose node
        rand_hbf = (brand(&br_state) % (max_freq - min_freq) )+ min_freq;
        for (j = 0; j < n; ++j)
            ps[j]->heartbeat_freq_ = rand_hbf;

        DATA() << "Heartbeat frequency set to: " << rand_hbf;
        // only stop non-masters
        if (ps[rand_ind]->i_am_master())
            rand_ind = (rand_ind + 1) % n;
        ps[rand_ind]->stop();

        twait { at_delay_msec(delay,make_event()); }
        ps[rand_ind]->start();
        twait { at_delay_msec(1500,make_event()); }
    }
    DATA () << "[True Drops] : " << Telemetry::true_drops_;
    DATA () << "[Perceived Drops] : " << Telemetry::perceived_drops_;

    tamer::break_loop();
}

int main() {
    // remove persistant files and log
    tamer::initialize();
    system("rm -rf *_persist log.txt");
    run(2000,100,1000);
    tamer::loop();
    tamer::cleanup();
    return 0;
}