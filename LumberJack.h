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
#include "TKey.h"

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
	 * selectVars    == XYZ..    -> vars that begin with XYZ
	 *                  ..XYZ    -> vars that end with XYZ
	 * 				    ..XYZ..  -> vars that contain XYZ
	 * 				    (REGEX) -> vars matched by regex expression REGEX
	 * var_ending   == new variable ending for selected vars
	 */
	void BestPVSelection(cTString& tree_name,
						 cTString& dimension_var,
						 cTString& outfile_name,
						 const char* selectVars="", // Default == all array-like
						 const char* var_ending="_flat");

	void dev();

private:
	bool inline contains(const std::vector<TString>&, cTString&) const;

	TFile *inFile = nullptr; // * = ROOT tradition
	TString input_filename, dimension_var;

	std::vector<TString> keep_trees;

	std::vector<TLeaf> double_leaves,
					   float_leaves,
					   int_leaves,
					   long_leaves,
					   ulong_leaves;

	ClassDef(LumberJack,1)
};


#endif
