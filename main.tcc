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

using namespace paxos;

tamed void wait_start(Paxos_Server* ps) {
    twait { tamer::at_delay(5,make_event()); }
    INFO() << "STARTING";
    ps->start();
}

tamed void run() {
    tvars { 
        int n(3),server_port_s(15800),paxos_port_s(15900),i;
        std::vector<Paxos_Server*> ps(n);
        Json config = Json::make_array();
        Paxos_Client* client;
        Json req,res;
    }
    for (i = 0; i < n; ++i) 
        config.push_back(Json::array(server_port_s + i, paxos_port_s + i));

    for (i = 0; i < n; ++i)
        ps[i] = new Paxos_Server(server_port_s + i, paxos_port_s + i, config);
    
    client = new Paxos_Client(config);
    twait { client->get_master(make_event()); }
    // stop master
    for (i = 0; i < n; ++i )
        if (ps[i]->who_is_master() == config[i][1].as_i()) {
            INFO() << "STOPPING " << config[i][1].as_i();
            ps[i]->stop();
            wait_start(ps[i]);
        }
    
    req = Json::array("test message",1,2,3,Json::null,Json::array());
    twait { client->make_request(req,make_event(res)); }
    INFO () << "decided : " << res;
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