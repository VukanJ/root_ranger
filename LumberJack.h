#ifndef LUMBERJACK_H
#define LUMBERJACK_H

#include <iostream>
#include <vector>
#include <typeinfo>
#include <string>
#include <algorithm>

#include "TString.h"
#include "TDirectoryFile.h"
#include "RooArgList.h"
#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TLeafI.h"
#include "TLeafF.h"
#include "TLeafD.h"
#include "TKey.h"

#include "LeafStore.h"

using cTString = const TString;

class LumberJack {
public:
	LumberJack(cTString& rootfile);
	virtual ~LumberJack();

	void addTree(cTString&);
	void changeFile(cTString& rootfile);

	/** BestPVSelection chooses first entry in array variables
	 * and writes new reduced tree with this selection
	 *
	 * tree_name     == Title of TTree where BPV selection should be applied
	 * dimension_var == Variable that stores the array dimensionality. DecayTreeFitter usually return "nPV"
	 * outfile_name  == Name of resulting reduced rootfile
	 * selectVars    == Vars to be kept. Default: All
	 *                  "XYZ*"   -> All branches that start with XYZ (Full wildcard support)
	 *                  any number of ellipsis possible
	 * 				    "(REGEX)" -> vars matched by regex expression REGEX
	 * var_ending   == new variable ending for selected vars
	 */
	void BestPVSelection(cTString& tree_name,
						 cTString& dimension_var,
						 cTString& outfile_name,
						 std::string selectVars="", // Default == all array-like
						 cTString var_ending=TString("_flat"));

	void dev();

private:
	void freeFileGracefully(TFile*);
	void prepareInputAndOutputTree(cTString& outfilename, cTString& target_tree, std::string& leaf_selection);
	void getListOfBranchesBySelection(std::vector<TLeaf*>&, TTree* target_tree, std::string& selection);

	TFile *inFile = nullptr, *outFile = nullptr; // * = ROOT tradition
	TTree *output_tree = nullptr; // Current output tree
	TString input_filename, dimension_var;

	std::vector<TString> keep_trees;

	std::vector<LeafStore<Double_t>> double_leaves;
	std::vector<LeafStore<Float_t>>  float_leaves;
	std::vector<LeafStore<Int_t>>    int_leaves;
	std::vector<LeafStore<Long_t>>   long_leaves;
	std::vector<LeafStore<ULong_t>>  ulong_leaves;

	ClassDef(LumberJack,1)
};

template<typename T>
bool inline contains(const std::vector<T>& vec, const T& elem)
{
    return std::find(std::begin(vec), std::end(vec), elem) != std::end(vec);
}

bool inline contains(const std::string& str, const char elem)
{
    return std::find(std::begin(str), std::end(str), elem) != std::end(str);
}

#endif
