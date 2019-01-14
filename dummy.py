import ROOT as R
from ROOT import gSystem
gSystem.Load("lumberjack.so")

lj = R.LumberJack("../test/0/DTT.root")

#lj.flattenTree("inclusive_Jpsi/DecayTree", "B0_Fit*", "")
#lj.BPVselection("inclusive_Jpsi/DecayTree", "B0_FitDaughtersConst*", "", "DecayTree")

#lj.Run("DTTSel.root")

lj.dev()
