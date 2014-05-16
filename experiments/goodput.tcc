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
const int MOD_STOP_DELAY = 500;
const int CHANGE_DELAY = 1000;
const int SAMPLE_DELAY = 50;
const int HEARTBEAT_INTERVAL = 280;
const int TIMEOUT_INTERVAL = 1000;
const int EFFICACY_WINDOW = 1000;
const int N_STEPS = 20;
// Experimental state
uint64_t failure_interval = 2000;
bool paused = false;


tamed void fail_node(std::vector<Paxos_Server*> ps, const int i)
{
    tvars { }
    ps[i]->stop();
    DATA() << "Stopped node "<<i;
    twait { at_delay_msec(TIMEOUT_INTERVAL+100, make_event()); }
    ps[i]->start();
    DATA() << "Started node "<<i;
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
        if (!paused) {
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
        }
        twait { at_delay_msec(failure_interval, make_event()); }
    }
}

tamed void run(const bool auto_) {
    tvars {
        int n(7),server_port_s(15800),paxos_port_s(15900),i;
        std::vector<Paxos_Server*> ps(n);
        Json config = Json::make_array();
        int master(15900);
        int master_index = 0;
        Paxos_Client* client;
        // Experiment variables
        int latency_ms,latency;
        int timeout_value;
        uint64_t time_start;
        int N_requests(20);
        Json req, res;
    }

    /*
        Experiment: goodput
        Goal: measure performance of either case by goodput
        Procedure:
            Modulate latency over time.
            Program automatically chooses best parameters for estimated values.
            Measure goodput directly.
    */

    // Server configuration and no-election startup
    for (i = 0; i < n; ++i) 
        config.push_back(Json::array(server_port_s + i, paxos_port_s + i));
    modcomm_fd::set_send_delay(10);
    modcomm_fd::set_recv_delay(0);
    DATA() << "Latency set to: " << 10;
    for (i=0; i<n; ++i) {
        ps[i] = new Paxos_Server(server_port_s+i, paxos_port_s+i, config, master);
        ps[i]->master_timeout_ = TIMEOUT_INTERVAL;
        // ps[i]->master_timeout_ = 1000000; // always wait for master to come back
        ps[i]->heartbeat_interval_ = HEARTBEAT_INTERVAL;
    }
    DATA() << "Heartbeat interval set to: " << HEARTBEAT_INTERVAL;
    twait { at_delay_msec(1000, make_event()); }

    client = new Paxos_Client(config);

    // Experiment
    Paxos_Server::enable_autopaxos = auto_;
    INFO() << "autopaxos set " << ((auto_) ? "ON" : "OFF" );
    failure_interval = 1200;
    randomly_fail(ps, n);

    req = Json::array("test message",1,2,3,Json::null,Json::array());
    for( latency = MOD_START_DELAY; latency <= MOD_STOP_DELAY; latency += (MOD_STOP_DELAY - MOD_START_DELAY)/N_STEPS ) {
        modcomm_fd::set_send_delay(latency);
        DATA() << "Latency set to: " << latency;
        paused = true;

        time_start = Telemetry::time();

        for (i = 0; i < N_requests; ++i)
            twait { client->make_request(req,make_event(res)); }

        DATA() << "[goodput]: " << N_requests << " sent in " << (Telemetry::time() - time_start);

        paused = false;
        twait { at_delay_msec(CHANGE_DELAY, make_event()); }
    }
    // twait { at_delay_msec(1000+CHANGE_DELAY*(N_STEPS+1), make_event()); }

    WARN() << "Breaking loop";
    tamer::break_loop();
}

static Clp_Option options[] = {
        { "autopaxos", 'a', 0, 0, Clp_Negate }
};

int main(int argc, const char** argv) {
        Clp_Parser* clp = Clp_NewParser(argc,argv,sizeof(options) / sizeof(options[0]), options);
        bool auto_ = false;
        while (Clp_Next(clp) != Clp_Done) {
            if (Clp_IsLong(clp,"autopaxos"))
                auto_ = !clp->negated;
        }
        // remove persistant files and log
        tamer::initialize();
        system("rm -rf *_persist log.txt");
        run(auto_);
        tamer::loop();
        tamer::cleanup();
        return 0;
}
