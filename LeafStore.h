#ifndef LEAFSTORE_H
#define LEAFSTORE_H

#include <vector>
#include <array>
#include <string>
#include <variant>

#include "TLeaf.h"

// https://root.cern.ch/doc/v610/classTBranch.html

enum LeafType {
    leaf_char,
	leaf_uchar,
	leaf_short,
	leaf_ushort,
	leaf_int,
	leaf_uint,
	leaf_float,
	leaf_double,
	leaf_long64,
	leaf_ulong64,
	leaf_bool
};

static const std::map<std::string, LeafType> LeafTypeFromStr 
{
    {"Char_t",    leaf_char},
    {"UChar_t",   leaf_uchar},
    {"Short_t",   leaf_short},
    {"UShort_t",  leaf_ushort},
    {"Int_t",     leaf_int},
    {"UInt_t",    leaf_uint},
    {"Float_t",   leaf_float},
    {"Double_t",  leaf_double},
    {"Long64_t",  leaf_long64},
    {"ULong64_t", leaf_ulong64},
    {"Bool_t",    leaf_bool}
};
// Datatype postfix
static const std::string DataTypeNamesShort = "BbSsIiFDLlO";

/* LeafBuffer: Class providing data buffer addresses for the event loop.
*  Since Leaves may contain arrays, the data buffers are stored as a vector
*/

template<typename T>
class LeafBuffer {
public:
    LeafBuffer() {
        buffer.reserve(1);
    }
    LeafBuffer(size_t length) : length(length) { 
        assert (length >= 1);
        buffer.reserve(length);
    }
    void increment(int offset) { 
        // Move next element to address &buffer[0] from where
        // the next element is read from when flattening arrays
        buffer[0] = buffer[offset]; 
    }
    std::vector<T> buffer; // Temporary buffer containing all array elements
    size_t length;         // Maximum array length of this leaf
};

using LeafBufferVar = std::variant<LeafBuffer<Char_t>,
                                   LeafBuffer<UChar_t>,
                                   LeafBuffer<Short_t>,
                                   LeafBuffer<UShort_t>,
                                   LeafBuffer<Int_t>,
                                   LeafBuffer<UInt_t>,
                                   LeafBuffer<Float_t>,
                                   LeafBuffer<Double_t>,
                                   LeafBuffer<Long64_t>,
                                   LeafBuffer<ULong64_t>>;

#endif
