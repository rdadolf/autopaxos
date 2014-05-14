#!/usr/bin/env python

import sys
import re
import numpy as np

import goodplot

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
#   Actual drop events => drop
#   Detected drop events => e_drop
#   Actual MTBF set points => mtbf
#   MTBF estimates => e_mtbf
# 
# Data format:
#   np.array( [ [time, event], ... ] )

# Relevant plotting events:
#   Actual latency changes => latency
#   Master RTT estimates => e_rtt
#   Heartbeat events => hb
#   Heartbeat frequency => hbfreq
# 
# Data format:
#   np.array( [ [time, event], ... ] )

CHANGE_DELAY = 4 # (in sec)

def parse_events(file):
  drop = []
  e_drop = []
  mtbf = []
  e_mtbf = []
  latency = []
  e_rtt = []
  hb = []
  e_pmet = []
  hbfreq = 0

  for line in [raw.strip() for raw in file.readlines()]:
    m = re.match('(\d+\.\d+).*DATA: Stopped node .*', line)
    if m is not None:
      drop.append( (float(m.group(1)), 1) )

    m = re.match('(\d+\.\d+).*DATA: Detected node drop event', line)
    if m is not None:
      e_drop.append( (float(m.group(1)), 1) )

    m = re.match('(\d+\.\d+).*DATA: Real MTBF set to: (\d+)', line)
    if m is not None:
      mtbf.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*DATA: MTBF Estimate: (\d+)', line)
    if m is not None:
      e_mtbf.append( (float(m.group(1)), float(m.group(2))) )
    m = re.match('(\d+\.\d+).*DATA: Latency set to: (\d+)', line)
    if m is not None:
      latency.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*DATA: Master RTT Estimate: (\d+)', line)
    if m is not None:
      e_rtt.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*INFO: got heartbeat.*', line)
    if m is not None:
      hb.append( (float(m.group(1)), 1) )

    m = re.match('(\d+\.\d+).*DATA: PMetric Estimate: ([-\d\.]+)', line)
    if m is not None:
      e_pmet.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('.*DATA: Heartbeat frequency set to: ([\d]+)', line)
    if m is not None:
      hbfreq = float(m.group(1))


  t0 = min(drop[0][0], e_drop[0][0], mtbf[0][0], e_mtbf[0][0], latency[0][0], e_rtt[0][0], hb[0][0])
  return ( np.array([(t-t0,x) for (t,x) in drop]),
           np.array([(t-t0,x) for (t,x) in e_drop]),
           np.array([(t-t0,x) for (t,x) in mtbf]),
           np.array([(t-t0,x) for (t,x) in e_mtbf]),
           np.array([(t-t0,x) for (t,x) in latency]),
           np.array([(t-t0,x) for (t,x) in e_rtt]),
           np.array([(t-t0,x) for (t,x) in hb]),
           np.array([(t-t0,x) for (t,x) in e_pmet]),
           hbfreq )

def plot_events(drop, e_drop, mtbf, e_mtbf, latency, e_rtt, hb, e_pmet, hbfreq):
  (fig,ax) = plt.subplots()

  # Turn mtbf into a step function
  mtbf_step_t = np.zeros((mtbf.shape[0]*2))
  mtbf_step_v = np.zeros((mtbf.shape[0]*2))
  mtbf_step_t[::2] = mtbf[:,0]
  mtbf_step_t[1:-1:2]= mtbf[1:,0]
  mtbf_step_t[-1] = mtbf[-1,0] + CHANGE_DELAY
  mtbf_step_v[::2] = mtbf[:,1]
  mtbf_step_v[1::2]= mtbf[:,1]
  # Turn latency into a step function
  latency_step_t = np.zeros((latency.shape[0]*2))
  latency_step_v = np.zeros((latency.shape[0]*2))
  latency_step_t[::2] = latency[:,0]
  latency_step_t[1:-1:2]= latency[1:,0]
  latency_step_t[-1] = latency[-1,0] + CHANGE_DELAY
  latency_step_v[::2] = latency[:,1]
  latency_step_v[1::2]= latency[:,1]

  # Plot actual MTBF and latency
  ax.plot( mtbf_step_t, mtbf_step_v, color=cb2[0], label='Actual MTBF' )
  ax.plot( latency_step_t, latency_step_v, color=cb2[1], label='Actual Latency' )
  ax.axvline(drop[0,0],ls=':',color='k',alpha=0.2, label='Node drops')
  for (t,_) in drop[1:]:
    ax.axvline(t,ls=':',color='k',alpha=0.2)
  # Plot mtbf_ and rtt_estimate_
  ax.plot( e_mtbf[:,0], e_mtbf[:,1]/1000., color=cb2[4], label='Estimated MTBF' )
  ax.plot( e_rtt[:,0], e_rtt[:,1]/2000., color=cb2[5], label='Estimated Latency' )
  # Overlay the estimated, theoretical, and measured policy metric
  axG = ax.twinx();
  axG.plot( e_pmet[:,0], e_pmet[:,1], color=cb2[3], label='Estimated PMetric')
  pmet = np.empty(e_pmet.shape)
  pmet[:,0] = e_pmet[:,0]
  T_hb = 280. # program constant
  T_bf = np.interp(pmet[:,0], mtbf_step_t, mtbf_step_v)
  T_to = 880. # program constant
  T_l  = np.interp(pmet[:,0], latency_step_t, latency_step_v)
  C_r  = 20. # approximation for cost of recovery
  C_hb = 2.  # approximation for cost of one heartbeat
  #print ( F_hb, F_mf, T_to, T_l, C_r, C_hb )
  pmet[:,1] = goodplot.pmetric( T_hb, T_bf, T_to, T_l, C_r, C_hb )
  axG.plot( pmet[:,0], pmet[:,1], color=cb2[2], label='Theoretical PMetric')
  # FIXME: empirical pmet

  # Polish off plot
  t_max = max(np.max(drop[:,0]),np.max(e_drop[:,0]),np.max(mtbf[:,0]), np.max(e_mtbf[:,0]), np.max(latency[:,0]),np.max(e_rtt[:,0]),np.max(hb[:,0]))
  ax.set_xlim((0,t_max))
  ax.set_xlabel(r'Time')
  ax.set_ylabel(r'ms')
  ax.set_title('Tracking policy accuracy')
  #ax.legend(loc=2)
  fig.tight_layout()
  return (fig,ax)


if __name__=='__main__':
  if len(sys.argv)>1:
    file=open(sys.argv[1])
  else:
    file=sys.stdin
  args = parse_events(file)
  (fig,ax) = plot_events(*args)
  fig.savefig('track_policy_plot.png')

