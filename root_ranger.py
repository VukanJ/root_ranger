from ROOT import gSystem
import ROOT
gSystem.Load("ranger.so")

class Ranger:
    def __init__(self, file):
        self.__ranger = ROOT.Ranger(file)
    
    def copy_tree(self, treename, dest="", branches="*", cut=""):
        self.__ranger.TreeCopy(treename, 
                               self.__extend_selection(branches), 
                               self.__extend_selection(cut),
                               dest)
    
    def flatten_tree(self, treename, flat_branches, branches="*", cut="", dest=""):
        self.__ranger.FlattenTree(treename,
                                  self.__extend_selection(branches),
                                  self.__extend_selection(flat_branches),
                                  self.__extend_selection(cut),
                                  dest)
    
    def bpv_selection(self, treename, bpv_branches, branches="*",  cut="", dest=""):
        self.__ranger.FlattenTree(treename,
                                  self.__extend_selection(branches),
                                  self.__extend_selection(bpv_branches),
                                  self.__extend_selection(cut),
                                  dest)

    def add_formula(formula_name, formula):
        # For example add_formula("B0_PT", "TMath::Sqrt(#B0_X**2+#B0_Y**2)")
        self.__ranger.addFormula(formula_name, formula)

    def reset(self):
        self.__ranger.reset()

    def change_file(self, file):
        self.__ranger.changeFile(file)

    def run(self, outfile):
        self.__ranger.Run(outfile)
    
    def __extend_selection(self, sel_list):
        if isinstance(sel_list, list):
            return "((" + ")|(".join(sel_list) + "))"
        else:
            return sel_list