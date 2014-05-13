#!/usr/bin/env python


import sys
import re
import numpy as np
import scipy.stats

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


F_hb = 1/(np.linspace(50,1000,100))

F_mf = 0.00003 # failure per second
T_to = 1000. # master timeout, in ms
T_l = 250. # latency, in ms
C_r = 10. # recovery cost, in packets
C_hb = 1. # heartbeat cost, in packets

def g(F_hb, F_mf, T_to, T_l, C_r):
  #false_fail = ((1./F_hb)+T_l) > T_to
  false_fail = scipy.stats.norm.cdf( (1./F_hb+T_l-T_to), loc=0, scale=100 )
  return (F_mf * T_to) / (2*( F_hb*C_hb + false_fail*C_r))

(fig,ax) = plt.subplots()
for T_to in np.linspace(500,3000,6):
  goodness = g(F_hb, F_mf, T_to, T_l, C_r)
  ax.plot(F_hb, goodness, label='timeout='+str(T_to))
  print 1./F_hb[np.argmax(goodness)], np.max(goodness)
ax.set_xlabel('Heartbeat frequency')
ax.set_ylabel('Goodness')
ax.legend()
fig.savefig('good.png')

