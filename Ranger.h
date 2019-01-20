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

#include "LeafBuffer.h"

// Buffer stores a list of leaves of a given datatype and a list of 
// indices of leaves in the first buffer, that have array dimension 
// and need to be flattened
template<typename L>
using Buffer = std::pair<std::vector<LeafBuffer<L>>, std::vector<int>>;

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
	void reset();

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
	// Utility methods
	void closeFile(TFile*);
	void clearBuffers();
	TLeaf* analyzeLeaves_FillLeafBuffers(TTree* input_tree, TTree* output_tree,
									     std::vector<TLeaf*>& all_leaves,
									     std::vector<TLeaf*>& bpv_leaves);
    
	template<typename L> Buffer<L>* getBuffer();

    template<typename L>
    void addLeaf(TString& name_before, TString& name_after,
				 TTree* tree_in, TTree* tree_out,
				 size_t buffer_size, bool assign_index);
	
	template<typename L>
	void inline incrementBuffer(int inc);

	void getListOfBranchesBySelection(std::vector<TLeaf*>&,
	                                  TTree* target_tree,
									  std::string selection);
	
	// Actual tree operations
	void SimpleCopy(const TreeJob&);
	void flattenTree(const TreeJob&);
	void BestPVSelection(const TreeJob&);
	void addFormulaBranch(const TreeJob&);

	std::vector<TreeJob> tree_jobs;

	FilePtr inFile, outFile;

	std::string input_filename;

	// Leaf buffer storage with indices of array-type leaves
    Buffer<Char_t>    leaf_buffers_B;
    Buffer<UChar_t>   leaf_buffers_b;
    Buffer<Short_t>   leaf_buffers_S;
    Buffer<UShort_t>  leaf_buffers_s;
    Buffer<Int_t>     leaf_buffers_I;
    Buffer<UInt_t>    leaf_buffers_i;
    Buffer<Float_t>   leaf_buffers_F;
    Buffer<Double_t>  leaf_buffers_D;
    Buffer<Long64_t>  leaf_buffers_L;
    Buffer<ULong64_t> leaf_buffers_l;

	ClassDef(Ranger,1)
};

template<typename L> 
Buffer<L>* Ranger::getBuffer()
{
	// Return buffer corresponding to datatype
	if constexpr (std::is_same<L, Char_t>::value)    return &leaf_buffers_B;
	if constexpr (std::is_same<L, UChar_t>::value)   return &leaf_buffers_b;
	if constexpr (std::is_same<L, Short_t>::value)   return &leaf_buffers_S;
	if constexpr (std::is_same<L, UShort_t>::value)  return &leaf_buffers_s;
	if constexpr (std::is_same<L, Int_t>::value)     return &leaf_buffers_I;
	if constexpr (std::is_same<L, UInt_t>::value)    return &leaf_buffers_i;
	if constexpr (std::is_same<L, Float_t>::value)   return &leaf_buffers_F;
	if constexpr (std::is_same<L, Double_t>::value)  return &leaf_buffers_D;
	if constexpr (std::is_same<L, Long64_t>::value)  return &leaf_buffers_L;
	if constexpr (std::is_same<L, ULong64_t>::value) return &leaf_buffers_l;
	return nullptr;
}

template<typename L> 
void Ranger::addLeaf(TString& name_before,
                     TString& name_after,
					 TTree* tree_in,
					 TTree* tree_out,
					 size_t buffer_size,
					 bool assign_index)
{
	Buffer<L>* lb_vec = getBuffer<L>();
	// Create leaf store, link addresses
	if (assign_index) {
		lb_vec->second.push_back(lb_vec->first.size());
	}
    lb_vec->first.emplace_back(std::move(LeafBuffer<L>(buffer_size)));

    tree_in->SetBranchAddress(name_before, &(lb_vec->first.back().buffer[0]));
    tree_out->Branch(name_after,           &(lb_vec->first.back().buffer[0]));
}

template<typename L>
void inline Ranger::incrementBuffer(int inc)
{
	Buffer<L>* buffer = getBuffer<L>();
	for (auto& leaf_idx : buffer->second) {
        buffer->first[leaf_idx].increment(inc);
    }
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
