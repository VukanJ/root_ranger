from root_ranger import Ranger
import time

ranger = Ranger("../test/0/DTT.root")
start = time.time()
#ranger.bpv_selection("inclusive_Jpsi/DecayTree", "B0_Fit*", dest="DecayTree")
ranger.flatten_tree("inclusive_Jpsi/DecayTree", "B0_Fit*", branches="B0*", dest="DecayTree")
ranger.run("Test_bpv.root")
end = time.time()


print("Took, ", end-start, " seconds")
#ranger2 = Ranger("../test/0/DTT.root")
#
#ranger2.run("Test_flat.root")