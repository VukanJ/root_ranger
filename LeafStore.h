#ifndef LEAFSTORE_H
#define LEAFSTORE_H

#include <vector>
#include <array>
#include <string>

#include "TLeaf.h"

// https://root.cern.ch/doc/v610/classTBranch.html
enum LeafType {
	BYTE_LEAF,
	UBYTE_LEAF,
	SHORT_LEAF,
	USHORT_LEAF,
	INT_LEAF,
	UINT_LEAF,
	FLOAT_LEAF,
	DOUBLE_LEAF,
	LONG_LEAF,
	ULONG_LEAF,
	BOOL_LEAF
};

// All data types supported by root
static const std::array<std::string, 11> DataTypeNames {
	"Char_t",
	"UChar_t",
	"Short_t",
	"UShort_t",
	"Int_t",
	"UInt_t",
	"Float_t",
	"Double_t",
	"Long64_t",
	"ULong64_t",
	"Bool_t"
};
// Datatype postfix
static const std::string DataTypeNamesShort = "BbSsIiFDLlO";

template <typename T>
class LeafStore {
public:
	LeafStore();
	LeafStore(TLeaf* leaf, LeafType leaf_type);

	T* operator&() noexcept;
	TString name(bool appendType=false);

	T data; // Temporary storage for data
	const TLeaf* leafptr;
	const LeafType type;
};

template<typename T>
LeafStore<T>::LeafStore() : data(0), leafptr(nullptr), type(INT_LEAF) { }

template<typename T>
LeafStore<T>::LeafStore(TLeaf* leaf, LeafType leaf_type) : data(0), leafptr(leaf), type(leaf_type) { }

template<typename T>
T* LeafStore<T>::operator&() noexcept
{
	return &data;
}

template<typename T>
TString LeafStore<T>::name(bool appendType){
	TString leafname = leafptr->GetName();
	if (appendType){
		leafname += '/';
		leafname += DataTypeNamesShort[type];
	}
	return leafname;
}

#endif
