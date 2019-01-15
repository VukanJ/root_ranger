import ROOT as R
from ROOT import gSystem
gSystem.Load("ranger.so")

lj = R.Ranger("../test/0/DTT.root")

#lj.flattenTree("inclusive_Jpsi/DecayTree", "B0_Fit*", "")
lj.BPVselection("inclusive_Jpsi/DecayTree", "*", "B0_Fit*", "B0_FitDaughtersConst_status==0", "DecayTree")

lj.Run("DTTSel.root")

#lj.dev()
