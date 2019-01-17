import ROOT as R
from ROOT import gSystem
gSystem.Load("ranger.so")


ranger = R.Ranger("../test/0/DTT.root")

ranger.BPVselection("inclusive_Jpsi/DecayTree", "*", "B0_Fit*", "", "DecayTree")
ranger.treeCopy("GetIntegratedLuminosity/LumiTuple", "", "", "LumiTuple")

ranger.Run("DTTSel.root")

#ranger.dev()
