from root_ranger import Ranger

ranger = Ranger("DTT.root")

bpv_writebranches = "B0_Fit*"
cuts = "B0_M>5500"

ranger.bpv_selection("inclusive_Jpsi/DecayTree", bpv_branches=bpv_writebranches, cut=cuts, dest="DecayTree")

ranger.run("DTT_new.root")
