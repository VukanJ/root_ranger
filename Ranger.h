#ifndef LUMBERJACK_H
#define LUMBERJACK_H

#include <iostream>
#include <algorithm>
#include <typeinfo>
#include <regex>
#include <vector>
#include <string>
#include <iomanip>
#include <set>
#include <unordered_set>
#include <map>
#include <memory>

#include "TString.h"
#include "TDirectoryFile.h"
#include "RooArgList.h"
#include "TFile.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TKey.h"

#include "LeafStore.h"

using cTString = const TString;

class Ranger {
public:
	Ranger(cTString& rootfile);
	virtual ~Ranger();

	void changeFile(cTString& rootfile);

	void treeCopy(cTString&, cTString& rename=TString(""));

	void treeCopySelection(cTString& treename,
	                       const std::string& branch_selection,
						   const std::string& cut_selection,
						   cTString& rename=TString(""));

	void flattenTree(cTString& treename,
					 const std::string& branch_selection,
					 const std::string& additional_branches_selection,
					 const std::string& cut_selection,
					 cTString& rename=TString(""));

	void BPVselection(cTString& treename,
					  const std::string& branch_selection,
					  const std::string& additional_branches_selection,
					  const std::string& cut_selection,
					  cTString& rename=TString(""));

	void Run(TString output_filename);

	void dev();

	enum Action {
		copytree,
		selection,
		flatten_tree,
		bpv_selection
	};

	struct TreeJob {
		/* TreeJob stores old and new name,
		*  as well as the action that
		*  should be performed on this tree
		*  Must be public for ROOT
		*/
		TString name, newname;
		std::string branch_selection,
		            branch_selection2,
								cut_selection;

		Action action;
	};

private:
	void freeFileGracefully(TFile*);
	void SimpleCopy(const TreeJob&);
	void analyzeLeaves_FillLeafBuffers(TTree* input_tree,
	                                   TTree* output_tree,
									   						 		 std::vector<TLeaf*>& all_leaves,
																	 	 std::vector<TLeaf*>& bpv_leaves);
	void getListOfBranchesBySelection(std::vector<TLeaf*>&,
	                                  TTree* target_tree,
									  								std::string selection);

	void flatten(const TreeJob&);
	void BestPVSelection(const TreeJob&);

	std::vector<TreeJob> tree_jobs;

	TFile *inFile = nullptr, *outFile = nullptr; // * = ROOT tradition

	TString input_filename;

	std::vector<LeafStore<Float_t>>  float_leaves;
	std::vector<LeafStore<Double_t>> double_leaves;
	std::vector<LeafStore<Int_t>>    int_leaves;
	std::vector<LeafStore<Long_t>>   long_leaves;
	std::vector<LeafStore<ULong_t>>  ulong_leaves;

	ClassDef(Ranger,1)
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
