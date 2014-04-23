#include "rpc_msg.hh"
#include "log.hh"
#include "network.hh"
#include "mpfd.hh"
#include "json.hh"
#include "clp.h"

#ifndef COMMAND_TYPE_ 
#define COMMAND_TYPE_ "start"
#endif

tamed void run(const char* hostname, int port) {
    tvars {
        tamer::fd cfd;
        msgpack_fd mpfd;
        struct in_addr hostip;
        RPC_Msg req, res;
    }

    {
        in_addr_t a = hostname ? inet_addr(hostname) : htonl(INADDR_LOOPBACK);
        if (a != INADDR_NONE)
            hostip.s_addr = a;
        else {
            struct hostent* hp = gethostbyname(hostname);
            if (hp == NULL || hp->h_length != 4 || hp->h_addrtype != AF_INET) {
                std::cerr << "lookup " << hostname << ": " << hstrerror(h_errno) << std::endl;
                return;
            }
            hostip = *((struct in_addr*) hp->h_addr);
        }
    }
    // connect
    twait { tamer::tcp_connect(hostip, port, make_event(cfd)); }
    if (!cfd) {
        std::cerr << "connect " << (hostname ? hostname : "localhost")
                  << ":" << port << ": " << strerror(-cfd.error()) << std::endl;
        return;
    }
    mpfd.initialize(cfd);

    req = RPC_Msg(Json::array(COMMAND_TYPE_));
    twait { mpfd.call(req, make_event(res.json())); }
    std::cout << "finished " << COMMAND_TYPE_<< ": " << res.content() << std::endl;
    // close out
    cfd.close();
}

static Clp_Option options[] = {
    { "port", 'p', 0, Clp_ValInt, 0 },
    { "host", 'h', 0, Clp_ValString, 0 }
};

int main(int argc, char** argv) {
    int port = 15800;
    String hostname = "localhost";
    Clp_Parser* clp = Clp_NewParser(argc,argv,sizeof(options) / sizeof(options[0]), options);
    while (Clp_Next(clp) != Clp_Done) {
        if (Clp_IsLong(clp,"port"))
            port = clp->val.i;
        else if (Clp_IsLong(clp,"host"))
            hostname = clp->vstr;
    }
    tamer::initialize();
    run(hostname.c_str(),port);
    tamer::loop();
    tamer::cleanup();
}