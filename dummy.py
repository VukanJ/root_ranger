import ROOT as R
from ROOT import gSystem
gSystem.Load("lumberjack.so")

lj = R.LumberJack("../test/0/DTT.root")
#lj.BPVselection("inclusive_Jpsi/DecayTree", "B0_Fit*", "nPV", "DecayTree")
lj.treeCopy("GetIntegratedLuminosity/LumiTuple", "Luminosity")
lj.treeCopy("inclusive_Jpsi/DecayTree", "DecayTree")

lj.Run("DTTSel.root")
