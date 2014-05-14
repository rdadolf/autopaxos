// -*- mode: c++ -*-
#include "mpfd.hh"
#include "rpc_msg.hh"
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include "paxos.hh"
#include "network.hh"
#include "telemetry.hh"
#include "policy.hh"
using namespace paxos;

int rand_int(double min, double max) {
    double ret = ((double) rand() / ((double)RAND_MAX+1)) * (max-min) + min;
    return (int) ret;
}

tamed void Paxos_Proposer::proposer_init (tamer::event<> done) {
    tvars {
        struct in_addr hostip;
        std::vector<int>::size_type i;
        std::vector<int> inds;
        bool conn;
    }
    assert(ports.size() == mpfd.size());
    for (i = 0; i < ports.size(); ++i)
        inds.push_back(i);
    while (!inds.empty()) {
        i = rand_int(0,inds.size());
        twait { client_init(hostname.c_str(),ports[inds[i]],cfd[inds[i]],mpfd[inds[i]],hostip,make_event(conn)); }
        if (conn)
            inds.erase(inds.begin() + i);
    }
    done();
}

tamed void Paxos_Proposer::client_init(const char* hostname, int port, tamer::fd& cfd, 
                        modcomm_fd& mpfd, struct in_addr& hostip,tamer::event<bool> done) {

    tvars {
        int s = 100;
    }
    // lookup hostname address
    {
        in_addr_t a = hostname ? inet_addr(hostname) : htonl(INADDR_LOOPBACK);
        if (a != INADDR_NONE)
            hostip.s_addr = a;
        else {
            struct hostent* hp = gethostbyname(hostname);
            if (hp == NULL || hp->h_length != 4 || hp->h_addrtype != AF_INET) {
                std::cout << "lookup " << hostname << ": " << hstrerror(h_errno) << std::endl;
                return;
            }
            hostip = *((struct in_addr*) hp->h_addr);
        }
    }

    twait { tamer::tcp_connect(hostip, port, make_event(cfd)); }
    while (!cfd) {
        INFO() << uid_ << " delaying to connect to " << port << ": " << s;
        twait { tamer::at_delay_msec(s,make_event()); }
        twait { tamer::tcp_connect(hostip, port, make_event(cfd)); }
        if (s <= 1000)
            s *= 2;
        else {
            std::cout << "timed out!" <<std::endl;
            done(false);
            return;
        }
    }
    mpfd.initialize(cfd);
    done(true);
}

tamed void Paxos_Proposer::send_to_all(RPC_Msg& req){
    tvars {
        std::vector<int>::size_type i;
        tamer::rendezvous<> r;
    }
    for (i = 0; i < ports.size(); ++i)
        mpfd[i].call(req,r.make_event(res[i].json()));
}

tamed void Paxos_Proposer::run_instance(Json _v,tamer::event<Json> done) {
    tvars {
        int n;
        std::vector<int>::size_type i;
        Json v;
        RPC_Msg req;
        tamer::rendezvous<bool> r;
        bool to;
    }
    set_vc(_v);
    INFO() << "starting instance " << uid_;;
start:
    v_o = Json::array();
    if(v_c[1].as_i() != me_->epoch_) { // replica is no longer master: shouldn't be sending 
        assert(v_c[1].as_i() < me_->epoch_);
        INFO() << "proposer's epoch number is behind in run_instance";
        v_o = Json::array("NACK");
        goto done;
    }
    propose(n,v,r.make_event(false));
    tamer::at_delay_sec(4,r.make_event(true)); // FIXME: 4 seconds is...?
    twait(r,to);
    if (to) { // timeout happened
        INFO() << "restarting after propose";;
        goto start;
    }
    r.clear();
    if (v_o.empty()) {
        v_o = v_c;
        assert(!v_c.empty());
    }

    accept(n,r.make_event(false));
    tamer::at_delay_sec(4,r.make_event(true)); // FIXME: 4 seconds is...?
    twait(r,to);
    if (to) {
        INFO() << "restarting after accept";;
        goto start;
    }
    
    req = RPC_Msg(Json::array(DECIDED,v_c[1].as_i(),n_p));

    send_to_all(req);
    INFO() << "decided " << v_o;;
done:
    *done.result_pointer() = v_o;
    done.unblock();
}


tamed void Paxos_Proposer::propose(int n, Json v, tamer::event<> done) {
    tvars { 
        RPC_Msg req;
        std::vector<int>::size_type i;
        tamer::rendezvous<int> r;
        int ret;
    }
    n_p = n_p + 1 + uid_; // FIXME : need uniqueifier 
    persist();
    n_o = a = 0;
    req = RPC_Msg(Json::array(PREPARE,v_c[1].as_i(),n_p));
    INFO() << "propose "<< uid_ <<": " << req.content();;

    for (i = 0; i < ports.size(); ++i)
        mpfd[i].call(req,r.make_event(i,res[i].json()));

    for (i = 0; i < (unsigned)f + 1; ++i) {
        twait(r,ret);
        if (res[ret].content() == Json::null) {
            i--;
            continue;
        }
        if (!res[ret].content()[0].is_i() 
            || res[ret].content()[0].as_i() != PREPARED) 
            --i;
        else {
            assert(res[ret].content()[1].is_i());
            n = res[ret].content()[1].as_i();
            v = res[ret].content()[2];
            if (n > n_o) {
                n_o = n;
                v_o = v;
            }
        }
    }

    done();
}

tamed void Paxos_Proposer::accept(int n, tamer::event<> done) {
    tvars {
        tamer::rendezvous<int> r;
        int ret;
        RPC_Msg req;
        std::vector<int>::size_type i;
    }
    n_p = std::max(n_o,n_p);
    persist();
    req = RPC_Msg(Json::array(ACCEPT,v_c[1].as_i(),n_p,v_o));
    INFO() << "send_accept " << uid_ << ": " << req.content();

    for (i = 0; i < ports.size(); ++i)
        mpfd[i].call(req,r.make_event(i,res[i].json()));

    for (i = 0; i < (unsigned)(f + 1); ++i) {
        twait(r,ret);
        //assert(res[ret].content()[0].is_i() && res[ret].content()[1].is_i());
        if( res[ret].content()[0].is_i() && res[ret].content()[1].is_i() ) {
          --i;
          continue; // will get a json assertion error if this is allowed to go to the "n =" line, 
                    // and the second member isn't an int
        }
        n = res[ret].content()[1].as_i();
        if (res[ret].content()[0].as_i() != ACCEPTED || n != n_p) // should not count this one
            --i;
    }

    done();
}

tamed void Paxos_Proposer::send_heartbeat() {
    tvars {
      RPC_Msg req;
      unsigned int n = ports.size();
      std::vector<RPC_Msg> resp(n);
      tamer::event<> e;
      std::vector<int>::size_type i;
      std::vector<int> results(n);
      tamer::rendezvous<int> r;
      struct timeval t;
      std::vector<uint64_t> t0s(n), t1s(n);
      uint64_t sum;
      Json d; // for pushing new data to telemetry
      int ret; // returned index on twait
      int diff;
    }

    n = ports.size();

    req = RPC_Msg(Json::array(HEARTBEAT,me_->epoch_,me_->master_));

    // Telemetry::live_event(0); // FIXME: wrong // this should be set by the heartbeat

    // Send to all (with a node timeout);
    for (i=0; i<n; ++i) {
      t0s[i] = Telemetry::time();
      mpfd[i].call( req, with_timeout_msec(me_->heartbeat_timeout_,
                                           r.make_event(i,resp[i].json())) );
    }
    // Wait for responses, and grab timing info
    for( i=0; i<n; ++i) {
      twait(r,ret);
      t1s[ret] = Telemetry::time();
      // if this has an epoch number that is behind
      INFO() << resp[ret].content();
      if (resp[ret].content()[0].is_s() 
          && resp[ret].content()[0].as_s() == "NACK") {

        assert(resp[ret].content().size() == 3 
               && resp[ret].content()[1].is_i() 
               && resp[ret].content()[2].is_i());
        me_->master_ = resp[ret].content()[1].as_i();
        me_->epoch_ = resp[ret].content()[2].as_i();
      }
    }

    // Average RTT's and update estimate for this round
    sum = 0;
    for( i=0; i<n; ++i ) {
      diff = t1s[i] - t0s[i];
      sum += diff;
      INFO() << "RTT measurement: " << diff;
      if (diff >= me_->heartbeat_timeout_ * 1000) {// make sure to convert heartbeat_timeout_ to same as Telemetry::time()
        DATA () << "RTT telemetry timeout. Node "<<i<<" dropped, RTT estimate set to " << diff;
        results[i] = ETIMEDOUT;
      } else 
        results[i] = 0;
    }
    Telemetry::update_rtt_estimate( sum/n );

    // Check status of all. FIXME: future functionality?
    for( i=0; i<n; ++i ) {
      if( results[i]==0 ) {
        // response received
        Telemetry::live_event(i);
        //INFO() << "Heartbeat received on port "<<ports[i]<< "(node "<<i<<")";
      } else if( results[i]== ETIMEDOUT ) {
        Telemetry::drop_event(t1s[i], i); // FIXME: second arg may be fragile?
        //INFO() << "Heartbeat timeout on port "<<ports[i]<< "(node "<<i<<")";
        //d = Json::array(t1s[i],false);
        //Telemetry::perceived_drops_.push_back(d);
      } else {
        // signal
      }
    }
}

tamed void Paxos_Acceptor::acceptor_init(tamer::event<> done) {
    tvars {
        tamer::fd cfd;
    }
    sfd_ = tamer::tcp_listen(port);
    if (sfd_)
        INFO() << "listening on port " << port;
    else
        ERROR() << "listen " << port << ": " << strerror(-sfd_.error());
    while (sfd_) {
        twait { sfd_.accept(make_event(cfd)); }
        handle_request(cfd);
    }
    done();
}

tamed void Paxos_Acceptor::handle_request(tamer::fd cfd) {
    tvars {
        modcomm_fd mpfd(cfd);
        RPC_Msg res,req;
        int n;
        Json v;
    }
    while (cfd) {
        twait { mpfd.read_request(make_event(req.json())); }
        if (!req.content().is_a() || req.content().size() < 2
            || !req.content()[0].is_i() || !req.content()[1].is_i()) {
            INFO() << "bad RPC: " << req.content();
            break;
        }
        if (me_->stopped_)
            continue;
        if (me_->epoch_ > req.content()[1].as_i()) {
            INFO() << "proposer's epoch number is behind in acceptor " << port << ": " << req.content();
            // respond with updated info
            res = RPC_Msg(Json::array("NACK",me_->master_,me_->epoch_),req);
            mpfd.write(res);
            continue;
        } if (me_->epoch_ < req.content()[1].as_i()) {// if I am behind, catch me up
            INFO() << "epoch behind in " << port << ": changing to " << req.content()[1].as_i();
            me_->epoch_ = req.content()[1].as_i();
            me_->master_ = req.content()[2].as_i();
            me_->master_change_.make_event().trigger();
        }
        // heartbeat
        me_->elect_me_.make_event(false).trigger();
        switch(req.content()[0].as_i()) {
            case PREPARE:
                INFO() << "receive_prepare " << port << " : " << req.content();;
                assert(req.content().size() == 3);
                n = req.content()[2].as_i();
                prepare(mpfd,req,n);
                break;
            case ACCEPT:
                INFO() << "receive_accept " << port << " : " << req.content();;
                assert(req.content().size() == 4 && req.content()[3].is_a());
                n = req.content()[2].as_i();
                v = req.content()[3];
                accept(mpfd,req,n,v);
                break;
            case DECIDED:
                INFO() << "receive_decided " << port << " : " << req.content();;
                assert(req.content().size() == 3);
                decided(mpfd,req);
                break;
            case HEARTBEAT:
                INFO() << "heartbeat: acceptor " << port;
                receive_heartbeat(mpfd,req);
                break;
            default:
                INFO() << "bad Paxos request: " << req.content();;
                break;
        }
    }

    cfd.close();
}

tamed void Paxos_Acceptor::prepare(modcomm_fd& mpfd, RPC_Msg& req,int n) {
    tvars {
        RPC_Msg res;
    }
    n_l = std::max(n_l,n);
    res = RPC_Msg(Json::array(PREPARED,n_a,v_a),req);
    persist();
    INFO() << "prepared " << port << ": " << res.content();
    mpfd.write(res);
}

tamed void Paxos_Acceptor::accept(modcomm_fd& mpfd, RPC_Msg& req, int n, Json v) {
    tvars {
        RPC_Msg res;
    }
    if (n >= n_l) {
        n_l = n_a = n;
        v_a = v;
    }

    res = RPC_Msg(Json::array(ACCEPTED,n_a),req);
    persist();
    mpfd.write(res);
}

tamed void Paxos_Acceptor::decided(modcomm_fd& mpfd, RPC_Msg& req) {
    tvars { 
        RPC_Msg res;
        Json v;
    }
    v = v_a;
    v_a = Json::make_array();
    n_a = 0;
    if (v[0].is_s()) {
    if (v[0].as_s() == "master") {
        assert(v[1].is_i() && v[2].is_i());
        me_->master_ = v[2].as_i();
        me_->epoch_ = v[1].as_i() + 1;
        me_->master_change_.make_event().trigger();
        if( me_->i_am_master() )
          DATA() << "MASTER " << port << " ALIVE";
        INFO() << "I, "<< me_->paxos_port_ << ", think master is : " << me_->master_;
    // } else (v[0].as_s() == "file") {
    }}
    res = RPC_Msg(Json::array(DECIDED,"ACK"),req);
    persist();
    mpfd.write(res);
}

tamed void Paxos_Acceptor::receive_heartbeat(modcomm_fd& mpfd,RPC_Msg req) {
    tvars { RPC_Msg res; }
    res = RPC_Msg(Json::array("ACK"),req);
    mpfd.write(res);
}


bool Paxos_Server::enable_autopaxos = false;
// config should have have the form Json::array(Json::array(paxos_server port,paxos_acceptor port))
Paxos_Server::Paxos_Server(int port, int paxos, Json config,int master) {
    listen_port_ = port;
    master_ = master;
    master_timeout_ = 500;
    heartbeat_interval_ = 200; // was originally master_timeout_/2
    heartbeat_timeout_ = master_timeout_;
    epoch_ = (master_ < 0) ? 0 : 1;
    config_ = config;
    stopped_ = false;
    paxos_port_ = paxos;
    run_server();
}
tamed void Paxos_Server::run_server() {
    tvars { Json j; }
    INFO () << "starting paxos server";
    // FIXME: paxos sync start event wait goes here
    twait { paxos_init(make_event()); }
    // twait { tamer::at_delay_msec(rand_int(0,1000),make_event()); }
    listen_for_master_change();
    INFO() << "Now listening for master change";
    if (master_ < 0) 
      twait { elect_me(make_event(j)); }
    else 
        master_change_.make_event().trigger();
    INFO() << "Past master check";
    listen_for_heartbeats();
    INFO() << "Now listening for heartbeats";
    policy_decision(); // updating my hbf and mto
    handle_new_connections();
}

tamed void Paxos_Server::paxos_init(tamer::event<> ev){
    tvars {
        int i;
        tamer::event<> e;
        std::vector<int> paxi;
    }
    Telemetry::n_nodes_ = config_.size();
    for (int i = 0; i < config_.size(); ++i) {
        assert(config_[i].is_a() && config_[i][0].is_i() && config_[i][1].is_i());
        paxi.push_back(config_[i][1].as_i());
    }
    acceptor_ = new Paxos_Acceptor(this,paxos_port_);
    acceptor_->acceptor_init(e);
    proposer_ = new Paxos_Proposer(this,paxos_port_,"localhost",paxi,paxi.size() / 2);
    twait { proposer_->proposer_init(make_event()); }
    ev();
}

tamed void Paxos_Server::listen_for_heartbeats() {
  tvars {
    bool em;
    Json j;
  }
  
  while (1) {
    elect_me_.clear();
    tamer::at_delay_msec(master_timeout_,elect_me_.make_event(true));
    twait(elect_me_,em);
    if (stopped_)
        continue;
    if (em) {
      elect_me_.clear();
      INFO() << "master timed out " << paxos_port_;
      Telemetry::master_drop_event(epoch_, Telemetry::time(), master_ - config_[0][1].as_i()); // FIXME: wrong
      master_ = -1;
      twait { elect_me(tamer::make_event(j)); }
    } else 
        INFO() << "got heartbeat " << paxos_port_;
  }
}

tamed void Paxos_Server::handle_new_connections()
{
  tvars {
    tamer::fd client_fd; // FIXME: only one simultaneous client. :(
    // tamer::fd listen_fd; 
  }
  
  listen_fd = tamer::tcp_listen(listen_port_);
  if( !listen_fd ) {
    fprintf(stderr, "Phat server unable to listen on %d: %s.\n", listen_port_, strerror(-listen_fd.error()));
    exit(-1);
  }

  INFO() << "Phat server listening on " << listen_port_;

  while(listen_fd) {
    twait { listen_fd.accept(make_event(client_fd)); }
    read_and_dispatch(client_fd);
  }
}

tamed void Paxos_Server::read_and_dispatch(tamer::fd client_fd)
{
  tvars {
    msgpack_fd mpfd;
    RPC_Msg request, reply;
    Json res;
  }

  mpfd.initialize(client_fd);
  while(mpfd) {
    twait{ mpfd.read_request(tamer::make_event(request.json())); }
    if(!mpfd)
      break;
    if( request.validate() ) {
        if (stopped_ && request.content()[0] != "start")
            continue;
      INFO() << "paxos server got: " << request.content();
      if( request.content()[0]=="get_master" ) {
        reply = RPC_Msg( get_master(), request );
      } else if( request.content()[0]=="request" ) {
        twait { receive_request(request.content()[1],make_event(res)); }
        reply = RPC_Msg(res,request);
      } else if( request.content()[0]=="stop" ) {
        stop();
        reply = RPC_Msg(Json::array("ACK"),request);
      } else if( request.content()[0]=="start" ) {
        stopped_ = false;
        reply = RPC_Msg(Json::array("ACK"),request);
        INFO() << "server " << listen_port_ << " starting.";
      // } else if( request.content()[0]=="<other thing here>" ) {
      } else {
        reply = RPC_Msg( Json::array(String("NACK")), request );
      }
    } else {
      reply = RPC_Msg( Json::array(String("NACK")), request );
    }
    twait { mpfd.write(reply, make_event()); }
  }
}

//Json Telemetry::true_drops_; // true drops are noted when the node is stopped using the stop message
//Json Telemetry::perceived_drops_;

uint64_t Telemetry::rtt_estimate_;

int64_t Telemetry::epoch_counter_ = 0;
int Telemetry::last_drop_ = 0;
int Telemetry::n_drops_ = 0;
uint64_t Telemetry::mtbf_ = 1000000;
uint64_t *Telemetry::drop_times_ = NULL;
int Telemetry::n_nodes_ = -1;
bool *Telemetry::drop_history_ = NULL;

void Paxos_Server::stop(){
    stopped_ = true;
    /*
    Json d = Json::make_array();
    d.push_back(Telemetry::time());
    if( i_am_master() ) {
        DATA() << "MASTER " << paxos_port_ << " DEAD";
        d.push_back(true);
    } else 
        d.push_back(false);
    Telemetry::true_drops_.push_back(d);
    */
    INFO() << "server " << listen_port_ << " stopping.";
}

tamed void Paxos_Server::elect_me(tamer::event<Json> ev) {
    tvars{ Json v; }
    v = Json::array("master",epoch_,paxos_port_);
    INFO() << "Elect: "<< paxos_port_;
    twait {proposer_->run_instance(v,make_event(*ev.result_pointer())); }
    ev.unblock();
}

tamed void Paxos_Server::listen_for_master_change() {
    while(1) {
        twait(master_change_);
        master_change_.clear();
        twait { beating_heart(make_event()); }
    }
}

tamed void Paxos_Server::beating_heart(tamer::event<> ev) {
    while (i_am_master()) {
        if (!stopped_) {
            INFO() << paxos_port_ << " Paxos_Server sending heartbeat. epoch: " << epoch_;
            // Do not wait. Let the heartbeats go on their own.
            // We need to send another round in hb_int_ ms,
            // regardless of whether these are done.
            proposer_->send_heartbeat();
        }
        twait { tamer::at_delay_msec(heartbeat_interval_, make_event()); }
    }
    ev();
}

Json Paxos_Server::get_master() {
    if( i_am_master() ) {
        return Json::array(String("ACK"));
    } else {
        int port = -1;
        for (int i = 0; i < config_.size(); ++i) {
            if (config_[i][1].as_i() == master_) {
                port = config_[i][0].as_i(); 
                break;
            }
        }
        return Json::array(String("NACK"),String("NOT_MASTER"),String("localhost"),port);
    }
}

tamed void Paxos_Server::receive_request(Json args, tamer::event<Json> ev) {
    tvars { 
        Json req;
    }
    INFO() << "receive_request";
    if (!i_am_master()) {
        req = get_master();
        *ev.result_pointer() = req;
        // swap(*ev.result_pointer(),req);
    } else {
        req = Json::array("blah",epoch_,args); // first argument, blah here, is the message type; "master" if master election
        twait { proposer_->run_instance(req,make_event(*ev.result_pointer())); }
    }
    ev.unblock();
}
tamed void Paxos_Server::policy_decision() {
  tvars {
    double p, T_hb, T_bf, T_to, T_l, C_r, C_hb;
    std::pair<double,double> params;
  }
  while (1) {
    twait { at_delay_msec(200,make_event()); } // FIXME: arbitrary delay
    T_hb = heartbeat_interval_;
    T_bf = Telemetry::mtbf_/1000.; // us -> ms
    T_to = master_timeout_;
    T_l  = Telemetry::rtt_estimate_/2000.; // rtt us -> latency ms
    C_r  = 20; // Magic Packet
    C_hb = 2;  // Cost Constants
    p = pmetric( T_hb, T_bf, T_to, T_l, C_r, C_hb );
    DATA() << "PMetric estimate before tuning: " << p;

    if( enable_autopaxos ) {
      INFO() << "Autotuning  parameters";
      // eventually for updated hbf and mto
      params = get_best_params( 100, 800, 20, // T_hb (min, max, step)
                                800,1600, 20, // T_to (min, max, step)
                                T_bf, T_l, C_r, C_hb );
      heartbeat_interval_ = params.first;
      master_timeout_ = params.second;
      DATA() << "Autopaxos chose T_hb = "<< heartbeat_interval_;
      DATA() << "Autopaxos chose T_to = "<< master_timeout_;

      p = pmetric( T_hb, T_bf, T_to, T_l, C_r, C_hb );
      DATA() << "Current PMetric estimate after tuning: " << p;
    }
  }
}
