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
#   Actual drop events => drop
#   Detected drop events => e_drop
#   Actual MTBF set points => mtbf
#   MTBF estimates => e_mtbf
# 
# Data format:
#   np.array( [ [time, event], ... ] )

def parse_events(file):
  drop = []
  e_drop = []
  mtbf = []
  e_mtbf = []

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

  t0 = min(drop[0][0], e_drop[0][0], mtbf[0][0], e_mtbf[0][0])
  return ( np.array([(t-t0,x) for (t,x) in drop]),
           np.array([(t-t0,x) for (t,x) in e_drop]),
           np.array([(t-t0,x) for (t,x) in mtbf]),
           np.array([(t-t0,x) for (t,x) in e_mtbf])
         )

def plot_events(drop, e_drop, mtbf, e_mtbf):
  (fig,ax) = plt.subplots()

  # Turn mtbf into a step function
  mtbf_step_t = np.zeros((mtbf.shape[0]*2))
  mtbf_step_v = np.zeros((mtbf.shape[0]*2))
  mtbf_step_t[::2] = mtbf[:,0]
  mtbf_step_t[1:-1:2]= mtbf[1:,0]
  mtbf_step_t[-1] = mtbf[-1,0] + 2.0 # CHANGE_DELAY (in sec)
  mtbf_step_v[::2] = mtbf[:,1]
  mtbf_step_v[1::2]= mtbf[:,1]

  ax.plot( mtbf_step_t, mtbf_step_v, color=cb2[0], label='Actual MTBF' )
  ax.plot( e_mtbf[:,0], e_mtbf[:,1]/1000., color=cb2[1], label='Estimated MTBF' )

  #for (t,_) in drop:
  #  ax.axvline(t,ls='--',color=rgb(196,196,196))
  for (t,_) in e_drop:
    ax.axvline(t,ls=':',color=rgb(196,196,196))
  t_max = max(np.max(drop[:,0]),np.max(e_drop[:,0]),np.max(mtbf[:,0]), np.max(e_mtbf[:,0]))
  ax.set_xlim((0,t_max))
  ax.set_ylim((0,1.2*max(np.max(mtbf[:,1]),np.max(e_mtbf[:,1]))/1000.))
  ax.set_xlabel(r'Time')
  ax.set_ylabel(r'MTBF (ms)')
  ax.set_title(r'Tracking performance of MTBF Estimator')
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
  fig.savefig('track_mtbf_plot.png')
  
