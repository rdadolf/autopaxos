#!/usr/bin/env python

import sys
import re
import numpy as np

################################################################################
# vvvvv MPL Boilerplate vvvvv
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import rcParams
import matplotlib.cm as cm
import matplotlib as mpl
def rgb(r,g,b):
    return (float(r)/256.,float(g)/256.,float(b)/256.)
# Modified colorbrewer2.org qualitative color table (8-paired)
cb2 = [ rgb(31,120,180), rgb(255,127,0), rgb(51,160,44), rgb(227,26,28), rgb(166,206,227), rgb(253,191,111), rgb(178,223,138), rgb(251,154,153) ]
rcParams['figure.figsize'] = (8,6)
rcParams['figure.dpi'] = 150
rcParams['axes.color_cycle'] = cb2
rcParams['lines.linewidth'] = 1
rcParams['axes.facecolor'] = 'white'
rcParams['font.size'] = 12
rcParams['patch.edgecolor'] = 'white'
rcParams['patch.facecolor'] = cb2[0]
rcParams['font.family'] = 'Helvetica'
rcParams['font.weight'] = 100
# ^^^^^ MPL Boilerplate ^^^^^
################################################################################

# Relevant plotting events:
#   Actual latency changes => latency
#   Master RTT estimates => e_rtt
#   Heartbeat events => hb
#   Heartbeat frequency => hbfreq
# 
# Data format:
#   np.array( [ [time, event], ... ] )

def parse_events(file):
  latency = []
  e_rtt = []
  hb = []
  hbfreq = 0

  for line in [raw.strip() for raw in file.readlines()]:
    m = re.match('(\d+\.\d+).*DATA: Latency set to: (\d+)', line)
    if m is not None:
      latency.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*DATA: Master RTT Estimate: (\d+)', line)
    if m is not None:
      e_rtt.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*INFO: got heartbeat.*', line)
    if m is not None:
      hb.append( (float(m.group(1)), 1) )

    m = re.match('.*DATA: Heartbeat frequency set to: (\d+)', line)
    if m is not None:
      hbfreq = float(m.group(1))

  t0 = min(latency[0][0], e_rtt[0][0], hb[0][0])
  return ( np.array([(t-t0,x) for (t,x) in latency]),
           np.array([(t-t0,x) for (t,x) in e_rtt]),
           np.array([(t-t0,x) for (t,x) in hb]),
           hbfreq )

def plot_events(latency, e_rtt, hb, hbfreq):
  (fig,ax) = plt.subplots()

  # Turn latency into a step function
  latency_step_t = np.zeros((latency.shape[0]*2))
  latency_step_l = np.zeros((latency.shape[0]*2))
  latency_step_t[::2] = latency[:,0]
  latency_step_t[1:-1:2]= latency[1:,0]
  latency_step_t[-1] = latency[-1,0] + 1.0 # CHANGE_DELAY (in sec)
  latency_step_l[::2] = latency[:,1]
  latency_step_l[1::2]= latency[:,1]

  ax.plot( latency_step_t, latency_step_l, color=cb2[0], label='Actual Latency' )
  ax.plot( e_rtt[:,0], e_rtt[:,1]/2000., color=cb2[1], label='Estimated Latency' )
  ax.axvline(hb[0,0],ls=':',color=rgb(196,196,196),label='Heartbeat Messages')
  for (t,_) in hb[1:]:
    ax.axvline(t,ls=':',color=rgb(196,196,196))
  print hbfreq
  ax.axhline(hbfreq,ls='--',color=rgb(196,196,196))
  t_max = max(np.max(latency[:,0]),np.max(e_rtt[:,0]),np.max(hb[:,0]))
  ax.set_xlim((0,t_max))
  ax.set_xlabel(r'Time')
  ax.set_ylabel(r'Latency (ms)')
  ax.set_title(r'Tracking performance of RTT Estimator')
  ax.legend(loc=2)
  fig.tight_layout()
  return (fig,ax)

if __name__=='__main__':
  if len(sys.argv)>1:
    file=open(sys.argv[1])
  else:
    file=sys.stdin
  (latency, e_rtt, hb, hbfreq) = parse_events(file)
  (fig,ax) = plot_events(latency, e_rtt, hb, hbfreq)
  fig.savefig('track_rtt_plot.png')
  
