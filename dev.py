from root_ranger import Ranger

ranger = Ranger("../test/0/DTT.root")
ranger.bpv_selection("inclusive_Jpsi/DecayTree", "B0_Fit*", dest="DecayTree")

ranger.run("Test.root")