import ROOT as R
from ROOT import gSystem
gSystem.Load("lumberjack.so")

lj = R.LumberJack("../test/0/DTT.root")

#lj.flattenTree("inclusive_Jpsi/DecayTree", "nPV", "B0_Fit", "")

#lj.Run("DTTSel.root")

lj.dev()
