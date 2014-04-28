#include "rpc_msg.hh"
#include "log.hh"
#include "network.hh"
#include "mpfd.hh"
#include "json.hh"
#include "clp.h"

tamed void run(const char* hostname, int port, String command, Json data) {
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
    INFO() << command << " trying to connect to " << hostname << ": " << port;
    twait { tamer::tcp_connect(hostip, port, make_event(cfd)); }
    if (!cfd) {
        std::cerr << "connect " << (hostname ? hostname : "localhost")
                  << ":" << port << ": " << strerror(-cfd.error()) << std::endl;
        return;
    }
    mpfd.initialize(cfd);
    req = RPC_Msg(Json::array(command,data));
    twait { mpfd.call(req, make_event(res.json())); }
    std::cout << "finished " << command<< ": " << res.content() << std::endl;
    // close out
    cfd.close();
}

static Clp_Option options[] = {
    { "port", 'p', 0, Clp_ValInt, 0 },
    { "host", 'h', 0, Clp_ValString, 0 },
    { "command" , 'c', 0, Clp_ValString, 0 },
    { "data" , 'd' , 0, Clp_ValString, 0 }
};

int main(int argc, char** argv) {
    int port = 15800;
    String hostname = "localhost";
    String command = "start";
    Json data = Json::null;
    Clp_Parser* clp = Clp_NewParser(argc,argv,sizeof(options) / sizeof(options[0]), options);
    while (Clp_Next(clp) != Clp_Done) {
        if (Clp_IsLong(clp,"port"))
            port = clp->val.i;
        else if (Clp_IsLong(clp,"host"))
            hostname = clp->vstr;
        else if (Clp_IsLong(clp,"command"))
            command = clp->vstr;
        else if (Clp_IsLong(clp,"data"))
            data = Json::parse(clp->vstr);
    }
    tamer::initialize();
    run(hostname.c_str(),port,command,data);
    tamer::loop();
    tamer::cleanup();
}