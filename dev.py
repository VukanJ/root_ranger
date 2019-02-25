from root_ranger import Ranger

ranger = Ranger("DTT.root")

bpv_writebranches = ["GpsYear", "nPV", "B0_FitDaughters*", "(B0_[M]+)", "B0_P[XY]"]
cuts              = ["B0_M > 5200", "TMath::Sqrt(B0_PX**2+B0_PY**2)>1000"]

ranger.copy_tree("inclusive_Jpsi/DecayTree", branches=bpv_writebranches, cut=cuts, dest="DecayTree")

ranger.run("DTT_new.root")
