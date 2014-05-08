#include "rpc_msg.hh"
#include "log.hh"
#include "network.hh"
#include "mpfd.hh"
#include "json.hh"
#include "clp.h"

#include "paxos.hh"

const int HEARTBEAT_INTERVAL = 150;

using namespace paxos;

tamed void run(int n, int ssp, int psp) {
    tvars { 
        std::vector<Paxos_Server*> ps(n);
        Json config = Json::make_array();
        int i;
    }

    for (i = 0; i < n; ++i) 
        config.push_back(Json::array(ssp + i, psp + i));

    for (i = 0; i < n; ++i)
        ps[i] = new Paxos_Server(ssp + i, psp + i, config);

}

static Clp_Option options[] = {
    { "nnodes", 'n', 0, Clp_ValInt, 0 },
    { "server_port", 's', 0, Clp_ValInt, 0 },
    { "paxos_port", 'p', 0, Clp_ValInt , 0 }
};

int main(int argc, char** argv) {
    Clp_Parser* clp = Clp_NewParser(argc,argv,sizeof(options) / sizeof(options[0]), options);
    int nnodes = 1;
    int server_start_port = 15800;
    int paxos_start_port = 15900;
    while (Clp_Next(clp) != Clp_Done) {
        if (Clp_IsLong(clp,"nnodes"))
            nnodes = clp->val.i;
        else if (Clp_IsLong(clp,"server_port"))
            server_start_port = clp->val.i;
        else if (Clp_IsLong(clp,"nnodes"))
            paxos_start_port = clp->val.i;
    }
    system("rm -rf *_persist log.txt");
    tamer::initialize();
    run(nnodes,server_start_port,paxos_start_port);
    tamer::loop();
    tamer::cleanup();
}