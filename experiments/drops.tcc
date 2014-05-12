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
#include "telemetry.hh"

using namespace paxos;

tamed void run() {
    tvars { 
        int n(3),server_port_s(15800),paxos_port_s(15900),master(15900),i;
        std::vector<Paxos_Server*> ps(n);
        Paxos_Client* client;
        Json config = Json::make_array();
        uint64_t data;
        RPC_Msg msg,res; 
        Telemetry telemetry_;
    }

    for (i = 0; i < n; ++i) 
        config.push_back(Json::array(server_port_s + i, paxos_port_s + i));

    for (i = 0; i < n; ++i)
        ps[i] = new Paxos_Server(server_port_s + i, paxos_port_s + i, config,master);

    twait {at_delay(5,make_event()); }
    ps[1]->stop();
    twait {at_delay(2,make_event()); }
    DATA () << "[True Drops] : " << Telemetry::true_drops_;
    DATA () << "[Perceived Drops] : " << Telemetry::perceived_drops_;

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