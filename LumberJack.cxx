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
  freeFileGracefully(inFile);
  freeFileGracefully(outFile);
}

void LumberJack::freeFileGracefully(TFile* fileptr)
{
  if (fileptr != nullptr){
    if(fileptr->IsOpen()){
      fileptr->Close();
    }
    delete fileptr;
  }
}

void LumberJack::changeFile(cTString& rootfile)
{
    input_filename = rootfile;

    freeFileGracefully(inFile);

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
  BestPVSelection("inclusive_Jpsi/DecayTree", "nPV", "DTTsel.root", "B0*");
}

void LumberJack::BestPVSelection(cTString& tree_name,
                                 cTString& dimension_var,
                                 cTString& outfile_name,
                                 std::string selectVars, // Default == all array-like
                                 cTString var_ending)
{
    prepareInputAndOutputTree(outfile_name, tree_name, selectVars);

    for (const auto& treestr : keep_trees){
        TTree* ttree = static_cast<TTree*>(inFile->Get(treestr));
        //tree->GetLeaf(dimension_var);

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

void LumberJack::prepareInputAndOutputTree(cTString& outfilename, cTString& target_tree, std::string& leaf_selection)
{
    freeFileGracefully(outFile);
    outFile = TFile::Open(outfilename, "RECREATE");

    for(cTString& tree_str : keep_trees){
        TTree *input_tree  = static_cast<TTree*>(inFile->Get(tree_str));
        TTree *output_tree = new TTree(tree_str, tree_str);
        // Get leaves of input tree
        std::vector<TLeaf*> selected_leaves;

        getListOfBranchesBySelection(selected_leaves, input_tree, leaf_selection);

        for(const auto& leaf : selected_leaves){
            std::string leaf_type = leaf->GetTypeName();
            if(leaf_type == "Float_t"){
                float_leaves.emplace_back(LeafStore<Float_t>(leaf, FLOAT_LEAF));
                output_tree->Branch(float_leaves.back().name().c_str(), &float_leaves.back(), float_leaves.back().name(true).c_str());
            }
            else if(leaf_type == "Double_t"){
                double_leaves.emplace_back(LeafStore<Double_t>(leaf, DOUBLE_LEAF));
            }
            else if(leaf_type == "Int_t"){
                int_leaves.emplace_back(LeafStore<Int_t>(leaf, INT_LEAF));
            }
            else if(leaf_type == "Long_t"){
                long_leaves.emplace_back(LeafStore<Long_t>(leaf, LONG_LEAF));
            }
            else if(leaf_type == "ULong_t"){
                ulong_leaves.emplace_back(LeafStore<ULong_t>(leaf, ULONG_LEAF));
            }
        }
        output_tree->Write();
        outFile->Close();
    }
}

void LumberJack::getListOfBranchesBySelection(std::vector<TLeaf*>& selected, TTree* target_tree, std::string& selection)
{
    // Collect branches that match regex
    TObjArray* leaf_list = target_tree->GetListOfLeaves();
    /*

    std::string regex_select;

    // Remove whitespace
    for(auto c = selection.begin(); c != selection.end();){
    c = (*c == ' ') ? selection.erase(c) : c + 1;
    }
    // Build regex
    if (selection.empty()){
    regex_select = R"(([\w\d_]+))";
    }
    else {
    if (selection.size() >= 2){
      if(*selection.begin() == '(' && selection.back() == ')'){ // User entered regex
        regex_select = selection;
      }
      else if (contains(selection, '*')){ // Selected vars by wildcard -> construct regex
        regex_select += "^";
        for(const auto& s : selection){
          regex_select += (s == '*') ? R"([\w\d_]+)" : std::string(1, s);
        }
        regex_select += "$";
      }
      else { // Literal variable name -> only one selected
        regex_select = "^" + selection + "$";
      }
    }
    }
    // Loop over branches, append if regex matches
    std::cout << regex_select << '\n';
    std::regex re(regex_select);

    for(const auto& leaf : *leaf_list){
    std::smatch match;
    std::cout << leaf->GetName() << '\n';
    std::regex_search(std::string(leaf->GetName()), match, re);
    //if(match.ready()) std::cout << leaf->GetName() << '\n';
    }
    */
    for(const auto& leaf : *leaf_list){
        selected.push_back(static_cast<TLeaf*>(leaf));
    }
}
