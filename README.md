# Autopaxos

A distributed consensus protocol which adapts to network conditions.

## Driving Hypothesis:

A [Paxos](http://research.microsoft.com/en-us/um/people/lamport/pubs/paxos-simple.pdf) which *measures* and *adapts* to its environment will perform better than one that does not.

## Installation:

        git clone <autopaxos>
        cd autopaxos
        git submodule init
        git submodule update
        cd mprpc
        ./bootstrap.sh
        ./configure CXX='g++ -std=gnu++0x'
        make
        cd ..
        make


