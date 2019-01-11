import ROOT as R
from ROOT import gSystem
gSystem.Load("lumberjack.so")

lj = R.LumberJack("../test/0/DTT.root")
lj.addTree("inclusive_Jpsi/DecayTree")

#lj.BestPVSelection("inclusive_Jpsi/DecayTree", "nPV", "/ceph/users/vjevtic/test/0/DTT_bpv.root")

lj.dev()
