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
#   Actual MTBF set points => mtbf
#   Estimated MTBF => e_mtbf
#   Actual latency changes => latency
#   Estimated latency => e_latency
#   Heartbeat events => hb
#   PMetric estimates => e_pmet
#   Measured traffic => traf
#   Master starts => starts
#   Master stops => stops
#   Node drop events => drop
#   Estimated drop events => e_drop
# 
# Data format:
#   np.array( [ [time, event], ... ] )

CHANGE_DELAY = 1 # (in sec)

def parse_events(file):
  mtbf = []
  e_mtbf = []
  latency = []
  e_latency = []
  hb = []
  e_pmet = []
  traf = []
  starts = []
  stops = []
  drop = []
  e_drop = []

  master = 0 # FIXME: Add a regex for master changes if/when that works

  for line in [raw.strip() for raw in file.readlines()]:
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
      e_latency.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*INFO: got heartbeat.*', line)
    if m is not None:
      hb.append( (float(m.group(1)), 1) )

    m = re.match('(\d+\.\d+).*DATA: RTT telemetry timeout.*', line)
    if m is not None:
      e_drop.append( (float(m.group(1)), 1) )

    m = re.match('(\d+\.\d+).*DATA: PMetric estimate before tuning: ([-\d\.]+)', line)
    if m is not None:
      e_pmet.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*DATA: Traffic transferred: ([\d]+)', line)
    if m is not None:
      traf.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*DATA: Started node ([\d]+)', line)
    if m is not None and int(m.group(2))==master:
      starts.append( (float(m.group(1)), 1) )

    m = re.match('(\d+\.\d+).*DATA: Stopped node ([\d]+)', line)
    if m is not None and int(m.group(2))==master:
      stops.append( (float(m.group(1)), 1) )


  print 'mtbf',len(mtbf)
  print 'e_mtbf',len(e_mtbf)
  print 'latency',len(latency)
  print 'e_latency',len(e_latency)
  print 'hb',len(hb)
  print 'e_pmet',len(e_pmet)
  print 'traf', len(traf)
  print 'starts', len(starts)
  print 'stops', len(stops)
  print 'drop', len(drop)
  print 'e_drop', len(e_drop)

  #t0 = min(mtbf[0][0], latency[0][0], e_pmet[0][0], hb[0][0], eff[0][0])
  t0 = min(e_mtbf[0][0], latency[0][0], e_latency[0][0], e_pmet[0][0], hb[0][0])
  tN = max(e_mtbf[-1][0], latency[-1][0], e_latency[-1][0], e_pmet[-1][0], hb[-1][0])
  return ( np.array([(t-t0,x) for (t,x) in mtbf]),
           np.array([(t-t0,x) for (t,x) in e_mtbf]),
           np.array([(t-t0,x) for (t,x) in latency]),
           np.array([(t-t0,x) for (t,x) in e_latency]),
           np.array([(t-t0,x) for (t,x) in hb]),
           np.array([(t-t0,x) for (t,x) in e_pmet]),
           np.array([(t-t0,x) for (t,x) in traf]),
           np.array([(t-t0,x) for (t,x) in starts]),
           np.array([(t-t0,x) for (t,x) in stops]),
           np.array([(t-t0,x) for (t,x) in drop]),
           np.array([(t-t0,x) for (t,x) in e_drop]),
           t0, tN
         )

def plot_events(mtbf, e_mtbf, latency, e_latency, hb, e_pmet, traf, starts, stops, drop, e_drop, t0, tN):
  (fig,ax) = plt.subplots()

  # Turn mtbf into a step function
  #mtbf_step_t = np.zeros((mtbf.shape[0]*2))
  #mtbf_step_v = np.zeros((mtbf.shape[0]*2))
  #mtbf_step_t[::2] = mtbf[:,0]
  #mtbf_step_t[1:-1:2]= mtbf[1:,0]
  #mtbf_step_t[-1] = mtbf[-1,0] + CHANGE_DELAY
  #mtbf_step_v[::2] = mtbf[:,1]
  #mtbf_step_v[1::2]= mtbf[:,1]
  # Turn latency into a step function
  latency_step_t = np.zeros((latency.shape[0]*2))
  latency_step_v = np.zeros((latency.shape[0]*2))
  latency_step_t[::2] = latency[:,0]
  latency_step_t[1:-1:2]= latency[1:,0]
  latency_step_t[-1] = latency[-1,0] + CHANGE_DELAY
  latency_step_v[::2] = latency[:,1]
  latency_step_v[1::2]= latency[:,1]

  # Plot actual MTBF and latency
  #ax.axhline(mtbf[0,1], color=cb2[0], label='Actual MTBF' )
  #ax.plot( mtbf_step_t, mtbf_step_v, color=cb2[0], label='Actual MTBF' )
  ax.plot( latency_step_t, latency_step_v, color=cb2[1], label='Actual Latency' )
  ax.axvline(e_drop[0,0],ls=':',color='k',alpha=0.2, label='Node drops') # FIXME: should we highlight master drops
  for (t,_) in e_drop[1:]:
    ax.axvline(t,ls=':',color='k',alpha=0.2)

  # Plot mtbf_ and rtt_estimate_
  ax.plot( e_mtbf[:,0], e_mtbf[:,1]/1000., color=cb2[4], label='Estimated MTBF' )
  ax.plot( e_latency[:,0], e_latency[:,1]/2000., color=cb2[5], label='Estimated Latency' )

  # Estimated pmetric
  axG = ax.twinx();
  axG.plot( e_pmet[:,0], e_pmet[:,1], color=cb2[3], label='Estimated PMetric')

  # Theoretical pmetric
  #pmet = np.empty(e_pmet.shape)
  #pmet[:,0] = e_pmet[:,0]
  #T_hb = 280. # program constant
  #T_bf = np.interp(pmet[:,0], mtbf_step_t, mtbf_step_v)
  #T_to = 880. # program constant
  #T_l  = np.interp(pmet[:,0], latency_step_t, latency_step_v)
  #C_r  = 20. # approximation for cost of recovery
  #C_hb = 2.  # approximation for cost of one heartbeat
  #print ( F_hb, F_mf, T_to, T_l, C_r, C_hb )
  #pmet[:,1] = goodplot.pmetric( T_hb, T_bf, T_to, T_l, C_r, C_hb )
  #axG.plot( pmet[:,0], pmet[:,1], color=cb2[2], label='Theoretical PMetric')

  # Measured efficacy
  # first, compute uptime per traffic window
  # traffic is measured directly
  # ratio is efficacy
  tl = 0 # last time
  uptimes = []
  for (t,x) in traf:
    # get just this window
    up = 0
    n = 0
    strts_ = filter(lambda (t_,_): t_ > tl and t_ < t,starts)
    stps_ = filter(lambda (t_,_): t_ > tl and t_ < t,stops)
    # no stops here
    if len(stps_) is 0:
      uptimes.append(t - tl)
      tl = t
      continue
    # difference at the length should be off by 1 at most
    assert (len(strts_) == len(stps_) or abs(len(strts_) - len(stps_)) == 1) 
    if len(strts_) is 0:
      uptimes.append(stps_[0][0] - tl)
      tl = stps_[0][0] # should only have length of 1
      continue
    # downtime is the time when a master is down until there is re-election
    j = 0
    for i,(ts,n) in enumerate(stps_):
      if j < len(strts_):
        tstrt = strts_[j][0]
        if tstrt < ts:
          up += ts - tstrt
          j += 1
        else :
          assert(j is 0) # this should only happen at the beginning
          up += ts - tl
      else:
        up += ts - strts_[j - 1][0] # this should only happen at the end

    uptimes.append(up / len(stps_))
    tl = t
  print "uptimes",len(uptimes)
  goodness = np.array(uptimes)/traf[:,0]
  ax.plot(traf[:,0],goodness * 1000,color='k',label='Goodness')

  # Polish off plot
  #t_max = max(np.max(mtbf[:,0]),np.max(latency[:,0]),np.max(hb[:,0]),np.max(e_pmet[:,0]),np.max(eff[:,0]))
  t_max = max(np.max(mtbf[:,0]),np.max(latency[:,0]),np.max(hb[:,0]),np.max(e_pmet[:,0]))
  ax.set_xlim((0,t_max))
  ax.set_xlabel(r'Time')
  ax.set_ylabel(r'ms')
  ax.set_title('Tracking policy accuracy')
  ax.legend(loc=2)
  fig.tight_layout()
  return (fig,ax)


if __name__=='__main__':
  if len(sys.argv)>1:
    file=open(sys.argv[1])
  else:
    file=sys.stdin
  args = parse_events(file)
  (fig,ax) = plot_events(*args)
  fig.savefig('analysis.png')

