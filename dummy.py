import ROOT as R
from ROOT import gSystem
gSystem.Load("ranger.so")

ranger = R.Ranger("../test/0/DTT.root")

#lj.flattenTree("inclusive_Jpsi/DecayTree", "B0_Fit*", "")
ranger.BPVselection("inclusive_Jpsi/DecayTree", "*", "B0_Fit*", "", "DecayTree")

ranger.Run("DTTSel.root")

#lj.dev()
