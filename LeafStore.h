#ifndef LEAFSTORE_H
#define LEAFSTORE_H

#include <vector>
#include <array>
#include <string>

#include "TLeaf.h"

// https://root.cern.ch/doc/v610/classTBranch.html

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

/* LeafStore: Class providing data buffer addresses for the event loop.
*  Since Leaves may contain arrays, the data buffers are stored as a vector
*/

template <typename T>
class LeafStore {
public:
	LeafStore()
	{
		buffer.reserve(1);
	}

	LeafStore(TLeaf* leaf, size_t length) : leafptr(leaf)
	{
		assert(length >= 1);
		buffer.reserve(length);
	}

	std::vector<T> buffer; // Temporary buffer containing all array elements

	const TLeaf *leafptr = nullptr;
};

#endif
