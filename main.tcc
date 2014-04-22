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

using namespace paxos;

tamed void run() {
    tvars { 
        int n(3),server_port_s(15800),paxos_port_s(15900),i;
        std::vector<Paxos_Server*> ps(n);
        Json config = Json::make_array();
        uint64_t data;
    }
    for (i = 0; i < n; ++i) 
        config.push_back(Json::array(server_port_s + i, paxos_port_s + i));

    modcomm_fd::set_delay(MODCOMM_DELAY); // FIXME

    for (i = 0; i < n; ++i)
        ps[i] = new Paxos_Server(server_port_s + i, paxos_port_s + i, config);

    // Wait for 5 seconds, measure bandwidth.
    // FIXME: trigger paxos sync start event
    twait { tamer::at_delay_msec(5000, make_event()); }
    tamer::break_loop(); // Stop the world.
    data = modcomm_fd::data_transferred();
    INFO() << "Session stats: " << data << " packets sent over 5 seconds.";

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
