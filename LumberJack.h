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

	void changeFile(cTString& rootfile);

	void treeCopy(cTString&, cTString& rename=TString(""));
	
	void treeCopySelection(cTString& treename,
	                       cTString& branch_selection,
						   cTString& cut_selection,
						   cTString& rename=TString(""));

	void BPVselection(cTString& treename,
	                  TString&  array_dimension,
					  cTString& branch_selection,
					  cTString& cut_selection,
					  cTString& rename=TString(""));

	void Print() const;
	void Run(TString output_filename);

	void dev();

	enum Action {
		simpleCopy,
		selectionCopy,
		selection,
		bpv_selection
	};

	struct TreeJob {
		/* Stores old and new name,
		*  as well as the action that
		*  should be performed on this tree
		*/
		TString name,
		        newname,
				dimension_var,
				branch_selection,
				cut_selection;

		Action action;
	};
private:
	void freeFileGracefully(TFile*);
	void SimpleCopy(cTString&, cTString&);
	void prepareOutputTree(cTString& outfilename, cTString& target_tree, std::string& leaf_selection);
	//void getListOfBranchesBySelection(std::vector<TLeaf*>&, TTree* target_tree, std::string& selection);


	void BestPVSelection(const TreeJob& tree_job);

	std::vector<TreeJob> tree_jobs;

	TFile *inFile = nullptr, *outFile = nullptr; // * = ROOT tradition
	//TTree *output_tree = nullptr; // Current output tree
	TString input_filename;

	//// std::vector<std::pair<TString, TString>> keep_trees;

	//std::vector<LeafStore<Float_t>>  float_leaves;
	//std::vector<LeafStore<Double_t>> double_leaves;
	//std::vector<LeafStore<Int_t>>    int_leaves;
	//std::vector<LeafStore<Long_t>>   long_leaves;
	//std::vector<LeafStore<ULong_t>>  ulong_leaves;

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
