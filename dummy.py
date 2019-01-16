import ROOT as R
from ROOT import gSystem
gSystem.Load("ranger.so")

ranger = R.Ranger("DTTSel.root")

#lj.flattenTree("inclusive_Jpsi/DecayTree", "B0_Fit*", "")

#ranger.BPVselection("inclusive_Jpsi/DecayTree", "*", "B0_Fit*", "", "DecayTree")

ranger.treeCopySelection("inclusive_Jpsi/DecayTree", "(B0_P[XY])", "", "DecayTree")
ranger.addFormula("MomentumAsymmetry", "(#B0_PX-#B0_PY)/(#B0_PX+#B0_PY)");

#ranger.Run("DTTSel.root")

#ranger.dev()
