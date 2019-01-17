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
#include "TFormula.h"
#include "TLeaf.h"
#include "TKey.h"

#include "LeafStore.h"

class Ranger {
public:
	Ranger(const TString& rootfile);
	virtual ~Ranger();
	
	void changeFile(const std::string& rootfile);

	// Tree job parser methods
	void treeCopy(const std::string& treename,
	              const std::string& branch_selection="",
				  const std::string& cut_selection="",
				  const std::string& rename="");


	void BPVselection(const std::string& treename,
					  const std::string& branch_selection,
					  const std::string& bpv_branch_selection,
					  const std::string& cut_selection="",
					  const std::string& rename="");

	void addFormula(const std::string& name, std::string formula);

	// Runs all specified Ranger jobs in sequence
	void Run(TString output_filename);

	// Reset Ranger jobs
	void clear();

	void dev();

	enum Action {
		copytree,
		selection,
		flatten_tree,
		bpv_selection,
		add_formula
	};

	struct TreeJob {
		/* TreeJob stores everything Ranger needs to
		*  know about an operation performed on a single tree
		*  Must be public for ROOT
		*/
		std::string inline operator[](const std::string& key) const {
			return opt.find(key)->second;
		}
		TString inline operator()(const std::string& key) const {
			return TString(opt.find(key)->second);
		}
		std::map<std::string, std::string> opt;
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
	void addFormulaBranch(const TreeJob&);

	std::vector<TreeJob> tree_jobs;

	TFile *inFile = nullptr, *outFile = nullptr; // * = ROOT tradition

	std::string input_filename;

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
