#include "Riostream.h"
#include "LumberJack.h"

LumberJack::LumberJack(cTString& rootfile)
  : input_filename(rootfile)
{
  TTree::SetMaxTreeSize(1000000000000);
  changeFile(input_filename);
}

LumberJack::~LumberJack()
{
  if (inFile != nullptr){
    inFile->Close();
    delete inFile;
  }
}

void LumberJack::changeFile(cTString& rootfile)
{
    input_filename = rootfile;

    if(inFile != nullptr){
        inFile->Close();
        delete inFile;
    }

    inFile = TFile::Open(rootfile, "READ");

    // Check whether root file is healthy
    if (!inFile->IsOpen()) {
      std::cerr << "Cannot open file " + input_filename << '\n';
      exit(1);
    }
    if (inFile->IsZombie()) {
      std::cerr << "Root file appears to be corrupted -> giving up\n";
      exit(1);
    }
}

void LumberJack::addTree(cTString& tree)
{
    keep_trees.push_back(tree);
}

void LumberJack::dev()
{

}

void LumberJack::BestPVSelection(cTString& tree_name,
                                 cTString& dimension_var,
                                 cTString& outfile_name,
                                 const char* selectVars, // Default == all array-like
                                 const char* var_ending)
{
    for (const auto& treestr : keep_trees){
        TTree* ttree = static_cast<TTree*>(inFile->Get(treestr));

        if(ttree->GetListOfBranches()->FindObject(dimension_var) == nullptr){
            std::cerr << "Array dim variable \"" << dimension_var << "\" not found in tree \"" << treestr << "\"\n";
            exit(1);
        }

        if(treestr == tree_name){ // Do BestPVSelection
            // Check if dimension branch exists


        }
        else {

        }
    }
}

bool inline LumberJack::contains(const std::vector<TString>& vec, cTString& elem) const
{
    return std::find(std::begin(vec), std::end(vec), elem) != std::end(vec);
}
