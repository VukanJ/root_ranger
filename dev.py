from root_ranger import Ranger

ranger = Ranger("../test/0/DTT.root")
ranger.add_formula("F", "TMath::Sqrt(#B0_M)")
ranger.copy_tree("inclusive_Jpsi/DecayTree", branches="B0_PX", dest="MyTree")
#ranger.bpv_selection("inclusive_Jpsi/DecayTree", "B0_Fit*", dest="MyTree")
ranger.run("DTT_flat.root")

# Invalid input tree names crashes program in a non-trivial way!