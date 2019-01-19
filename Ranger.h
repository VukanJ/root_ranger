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
#include "TFile.h"
#include "TTree.h"
#include "TFormula.h"
#include "TLeaf.h"

#include "LeafStore.h"

using TreePtr = std::unique_ptr<TTree>;
using FilePtr = std::unique_ptr<TFile>;

class Ranger {
public:
	Ranger(const TString& rootfile);
	virtual ~Ranger();
	
	void changeFile(const std::string& rootfile);

	// Tree job parser methods
	void TreeCopy(const std::string& treename,
	              const std::string& branch_selection="",
				  const std::string& cut_selection="",
				  const std::string& rename="");


    void FlattenTree(const std::string& treename,
					 const std::string& branch_selection,
					 const std::string& flat_branch_selection,
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
	void closeFile(TFile*);
	void SimpleCopy(const TreeJob&);
	TLeaf* analyzeLeaves_FillLeafBuffers(TTree* input_tree,
									     std::vector<TLeaf*>& all_leaves,
									     std::vector<TLeaf*>& bpv_leaves);
    
    template<typename L>
    void addLeaf(TString& name_before, TString& name_after, size_t len);

	void getListOfBranchesBySelection(std::vector<TLeaf*>&,
	                                  TTree* target_tree,
									  std::string selection);

	void flattenTree(const TreeJob&);
	void BestPVSelection(const TreeJob&);
	void addFormulaBranch(const TreeJob&);

	std::vector<TreeJob> tree_jobs;

	FilePtr inFile, outFile; 

    TTree* input_tree;
	TreePtr output_tree;

	std::string input_filename;

    std::vector<LeafBufferVar> leaf_buffers;
    // memorize which leaves need to be flattened (array increment)
    std::map<std::string, std::vector<int>> update_flat_leaves; 

	ClassDef(Ranger,1)
};

template<typename L> 
void Ranger::addLeaf(TString& name_before, TString& name_after, size_t buffer_size)
{
    leaf_buffers.emplace_back(std::move(LeafBuffer<L>(buffer_size)));

	auto lb = std::get_if<LeafBuffer<L>>(&leaf_buffers.back());

    input_tree->SetBranchAddress(name_before, &(lb->buffer[0]));
    output_tree->Branch(name_after,           &(lb->buffer[0]));
}

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
