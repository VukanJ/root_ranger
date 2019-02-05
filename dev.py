from root_ranger import Ranger

ranger = Ranger("DTT.root")

ranger.bpv_selection("inclusive_Jpsi/DecayTree", bpv_branches="B0_Fit*", dest="DecayTree")

ranger.run("DTT_flat.root")
