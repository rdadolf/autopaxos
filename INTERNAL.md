# Code goals
KEEP:

- paxos
- elections
- modcomm
- logging

TOSS:

- clients
- puppet
- phat
- rpc_msg

ADD:

- fake client requests (omniscient injection)
- timing measurment
- bandwidth measurement (modcomm)
- fake server death (state reset)
- server start event

----

TODO:
- create monotonic experiments (goodness vs. latency, vs. master drop)

FUTURE:
- make modcomm traffic counter per-instance (not static)
