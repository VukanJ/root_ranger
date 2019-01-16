import ROOT as R
from ROOT import gSystem
gSystem.Load("ranger.so")
print("START")

ranger = R.Ranger("../test/0/DTT.root")

ranger.treeCopy("inclusive_Jpsi/DecayTree", "B0_P[XYT]", "", "DecayTree")
ranger.addFormula("BMeson_PT", "TMath::Sqrt(#B0_PX**2+#B0_PY**2)")
ranger.addFormula("BMeson_PT_diff", "#BMeson_PT-#B0_PT")
for p in range(3, 11):
    #print("TMath::Power(#B0_PT, 1.0/"+str(p)+".0)")
    ranger.addFormula("BMeson_PT_Sqrt_"+str(p), "TMath::Power(#B0_PT, 1.0/"+str(p)+".0)")

print("RUN")
ranger.Run("DTTSel.root")

#ranger.dev()
