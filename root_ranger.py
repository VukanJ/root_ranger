import os
from tqdm import tqdm
import ROOT
from ROOT import gSystem, gInterpreter

thispath = os.path.abspath(os.path.dirname(__file__))
gInterpreter.ProcessLine('#include "{0}/Ranger.h"'.format(thispath))
gInterpreter.ProcessLine('#include "{0}/LeafBuffer.h"'.format(thispath))
gSystem.Load(os.path.join(thispath, 'ranger.so'))


class Ranger:
    def __init__(self, file):
        self.__ranger = ROOT.Ranger(file)

    def copy_tree(self, treename, dest='', branches='*', cut=''):
        """Copies a TTree to a new file using a branch selection and an optional cut"""
        self.__ranger.TreeCopy(treename,
                               self.__construct_regex(branches),
                               self.__parse_cut(cut),
                               dest)

    def flatten_tree(self, treename, flat_branches, branches='*', cut='', dest=''):
        """Uses the leaf counter variable associated with the branches in flat_branches
           to reduce the dimensionality of these leaves. If multiple different leaf counters
           are used, they need to have the same contents."""
        self.__ranger.FlattenTree(treename,
                                  self.__construct_regex(branches),
                                  self.__construct_regex(flat_branches),
                                  self.__parse_cut(cut),
                                  dest)

    def bpv_selection(self, treename, bpv_branches, branches='*', cut='', dest=''):
        """If branch elements have array dimension, bpv_selection only selects the first
           element and discards the rest. This is often required for a bpv selection if
           the DecayTreeFitter is used.
        """
        self.__ranger.BPVselection(treename,
                                   self.__construct_regex(branches),
                                   self.__construct_regex(bpv_branches),
                                   self.__parse_cut(cut),
                                   dest)

    def add_selection(self, treename, dest='', branches='*', cut='', flat_branches='', bpv_branches=''):
        """Copies a TTree to a new file using a branch selection and an optional cut.
        If flat_branches is used, the leaf counter variable associated with the branches in
        flat_branches is used to reduce the dimensionality of these leaves. If multiple
        different leaf counters are used, they need to have the same contents.
        If bpv_branches is used and branch elements have array dimension, bpv_selection only
        selects the first element and discards the rest. This is often required for a bpv
        selection if the DecayTreeFitter is used.
        flat_branches and bpv_branches cannot be used at the same time."""
        if flat_branches and bpv_branches:
            raise ValueError('Flatten and best PV selection is not possible for the same target tree.')
        elif flat_branches:
            self.__ranger.FlattenTree(treename,
                                      self.__construct_regex(branches),
                                      self.__construct_regex(flat_branches),
                                      self.__parse_cut(cut),
                                      dest)
        elif bpv_branches:
            self.__ranger.BPVselection(treename,
                                       self.__construct_regex(branches),
                                       self.__construct_regex(bpv_branches),
                                       self.__parse_cut(cut),
                                       dest)
        else:
            self.__ranger.TreeCopy(treename,
                                   self.__construct_regex(branches),
                                   self.__parse_cut(cut),
                                   dest)

    def add_formula(self, formula_name, formula):
        """Adds a formula to the formula buffer that is evaluated in the next writing step.
           Branch names must start with '#'
        """
        self.__ranger.addFormula(formula_name, formula)

    def reset(self):
        """Resets all root_ranger tree jobs"""
        self.__ranger.reset()

    def set_input_file(self, file):
        """Sets a new input file"""
        self.__ranger.setInputFile(file)

    def run(self, outfile):
        """Runs all previously defined selections in sequence"""
        self.__ranger.Run(outfile)

    def run_multiple(self, infiles, outfiles):
        """Runs all previously defined selections in sequence on a list of root files"""
        assert len(infiles) == len(outfiles)
        for infile, outfile in tqdm(zip(infiles, outfiles), total=len(infiles)):
            self.__ranger.setInputFile(infile)
            self.__ranger.Run(outfile)

    def __parse_cut(self, cut):
        """If cuts are given as a list, they are joined by logical AND"""
        return ('(' + ')&&('.join(cut) + ')') if isinstance(cut, list) else cut

    def __construct_regex(self, sel_list):
        if isinstance(sel_list, list):
            return '|'.join(sel_list)
        return sel_list
