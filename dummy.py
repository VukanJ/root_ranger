import ROOT as R
from ROOT import gSystem
gSystem.Load("ranger.so")
print("START")

ranger = R.Ranger("../test/0/DTT.root")

ranger.treeCopy("inclusive_Jpsi/DecayTree", "B0_P*", "", "DecayTree")
#ranger.addFormula("MomentumAsymmetry", "(#B0_PX-#B0_PY)/(#B0_PX+#B0_PY)");

print("RUN")
ranger.Run("DTTSel.root")

#ranger.dev()
