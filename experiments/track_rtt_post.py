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
rcParams['figure.figsize'] = (6, 4)
rcParams['figure.dpi'] = 150
rcParams['axes.color_cycle'] = cb2
rcParams['lines.linewidth'] = 1
rcParams['axes.facecolor'] = 'white'
rcParams['font.size'] = 14
rcParams['patch.edgecolor'] = 'white'
rcParams['patch.facecolor'] = cb2[0]
rcParams['font.family'] = 'Helvetica'
rcParams['font.weight'] = 300
# ^^^^^ MPL Boilerplate ^^^^^
################################################################################

# Relevant plotting events:
#   Actual latency changes => latency
#   Master RTT estimates => e_rtt
#   Heartbeat events => hb
# 
# Data format:
#   np.array( [ [time, event], ... ] )

def parse_events(file):
  latency = []
  e_rtt = []
  hb = []

  for line in [raw.strip() for raw in file.readlines()]:
    m = re.match('(\d+\.\d+).*DATA: Latency set to: (\d+)', line)
    if m is not None:
      latency.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*DATA: Master RTT Estimate: (\d+)', line)
    if m is not None:
      e_rtt.append( (float(m.group(1)), float(m.group(2))) )

    m = re.match('(\d+\.\d+).*INFO: got heartbeat.*', line)
    if m is not None:
      e_rtt.append( (float(m.group(1)), 1) )

  return (np.array(latency), np.array(e_rtt), np.array(hb))

def plot_events(latency, e_rtt, hb):
  (fig,ax) = plt.subplots()

  ax.plot( latency[:,0], latency[:,1], color=cb2[0] )
  ax.plot( e_rtt[:,0], e_rtt[:,1]/2000., color=cb2[1] )
  for (t,_) in hb:
    ax.axvline(t,ls='--',color='gray',alpha=0.5)
  ax.set_xlabel(r'Time $\to$')
  ax.set_ylabel(r'Latency ($\mu$s)')
  ax.set_title(r'Tracking performance of RTT Estimator')
  fig.tight_layout()
  return (fig,ax)

if __name__=='__main__':
  if len(sys.argv)>1:
    file=open(sys.argv[1])
  else:
    file=sys.stdin
  (latency, e_rtt, hb) = parse_events(file)
  (fig,ax) = plot_events(latency, e_rtt, hb)
  fig.savefig('track_rtt_plot.png')
  
