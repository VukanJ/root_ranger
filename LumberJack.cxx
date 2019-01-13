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
    std::cout << "Changing file\n";
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

void LumberJack::treeCopy(cTString& tree, cTString& rename)
{
    tree_jobs.push_back({tree,
                         rename == "" ? tree : rename,
                         "", "", "",
                         Action::simpleCopy});
}

void LumberJack::treeCopySelection(cTString& treename,
                                   cTString& branch_selection,
                                   cTString& cut_selection,
                                   cTString& rename)
{
    tree_jobs.push_back({treename,
                         rename == "" ? treename : rename,
                         "",
                         branch_selection,
                         cut_selection,
                         Action::selectionCopy});
}

void LumberJack::BPVselection(cTString& treename,
                              TString&  array_dimension,
                              cTString& branch_selection,
                              cTString& cut_selection,
                              cTString& rename=TString(""))
{
    tree_jobs.push_back({treename,
                         rename == "" ? treename : rename,
                         array_dimension,
                         branch_selection,
                         cut_selection,
                         Action::bpv_selection});
}

void LumberJack::dev()
{
  for(auto& t : tree_jobs){
      std::cout << t.name << ' ' << t.newname << ' ' << t.dimension_var << ' ' << t.selection << ' ' << t.action << '\n';
  }
}

void LumberJack::Print() const
{
    // Print what LumberJack is about to do if Run() is called
    std::cout << "LumberJack job for " << input_filename << ":\n";
    for(const auto& tree_job : tree_jobs){
        switch(tree_job.action){
        case Action::simpleCopy:
            std::cout << "\t Copy tree:\t" << tree_job.name;
            if(tree_job.newname != ""){
                std::cout << " --> " << tree_job.newname;
            }
            break;
        case Action::selectionCopy:
            std::cout << "\t Copy tree with selection:\t" << tree_job.name;
            if(tree_job.newname != ""){
                std::cout << " --> " << tree_job.newname;
            }

            break;
        case Action::bpv_selection:
            std::cout << "\t Apply BPV selection:\t" << tree_job.name;
            if(tree_job.newname != ""){
                std::cout << " --> " << tree_job.newname;
            }
            break;
        }
        std::cout << '\n';
    }
}

void LumberJack::Run(TString output_filename)
{
    // Create output file
    if(!output_filename.EndsWith(".root")){
        output_filename += ".root";
    }
    freeFileGracefully(outFile);
    outFile = TFile::Open(output_filename, "RECREATE");

    for(const auto& tree_job : tree_jobs){
        switch(tree_job.action){
        case Action::simpleCopy: SimpleCopy(tree_job.name, tree_job.newname); break;
        default: break;
        }
    }
}

void LumberJack::SimpleCopy(cTString& tree_in, cTString& tree_out)
{
    TTree* input_tree = static_cast<TTree*>(inFile->Get(tree_in));
    input_tree->SetBranchStatus("*", 1);

    TTree* output_tree = input_tree->CloneTree();

    output_tree->Write("", TObject::kOverwrite); // Disable Autosave backups
}

void LumberJack::BestPVSelection(const TreeJob& tree_job)
{

}

void LumberJack::prepareOutputTree(cTString& target_tree, std::string& leaf_selection)
{
    for(const auto& treename : keep_trees){
        TTree *input_tree  = static_cast<TTree*>(inFile->Get(treename.first));
        TTree output_tree(treename.second, treename.second);
        // Get leaves of input tree
        std::vector<TLeaf*> selected_leaves;

        getListOfBranchesBySelection(selected_leaves, input_tree, leaf_selection);

        for(const auto& leaf : selected_leaves){
            std::string leaf_type = leaf->GetTypeName();
            if(leaf_type == "Float_t"){
                float_leaves.emplace_back(LeafStore<Float_t>(leaf, FLOAT_LEAF));
                output_tree.Branch(float_leaves.back().name(), &float_leaves.back(), float_leaves.back().name(true));
            }
            else if(leaf_type == "Double_t"){
                double_leaves.emplace_back(LeafStore<Double_t>(leaf, DOUBLE_LEAF));
                output_tree.Branch(double_leaves.back().name(), &double_leaves.back(), double_leaves.back().name(true));
            }
            else if(leaf_type == "Int_t"){
                int_leaves.emplace_back(LeafStore<Int_t>(leaf, INT_LEAF));
                output_tree.Branch(int_leaves.back().name(), &int_leaves.back(), int_leaves.back().name(true));
            }
            else if(leaf_type == "Long_t"){
                long_leaves.emplace_back(LeafStore<Long_t>(leaf, LONG_LEAF));
                output_tree.Branch(long_leaves.back().name(), &long_leaves.back(), long_leaves.back().name(true));
            }
            else if(leaf_type == "ULong_t"){
                ulong_leaves.emplace_back(LeafStore<ULong_t>(leaf, ULONG_LEAF));
                output_tree.Branch(ulong_leaves.back().name(), &ulong_leaves.back(), ulong_leaves.back().name(true));
            }
        }
        output_tree.Write();
    }
    for(auto& l : int_leaves){
        std::cout << l.name(true) << '\n';
    }
}

//void LumberJack::getListOfBranchesBySelection(std::vector<TLeaf*>& selected, TTree* target_tree, std::string& selection)
//{
//    // Collect branches that match regex
//    TObjArray* leaf_list = target_tree->GetListOfLeaves();
//    /*
//
//    std::string regex_select;
//
//    // Remove whitespace
//    for(auto c = selection.begin(); c != selection.end();){
//    c = (*c == ' ') ? selection.erase(c) : c + 1;
//    }
//    // Build regex
//    if (selection.empty()){
//    regex_select = R"(([\w\d_]+))";
//    }
//    else {
//    if (selection.size() >= 2){
//      if(*selection.begin() == '(' && selection.back() == ')'){ // User entered regex
//        regex_select = selection;
//      }
//      else if (contains(selection, '*')){ // Selected vars by wildcard -> construct regex
//        regex_select += "^";
//        for(const auto& s : selection){
//          regex_select += (s == '*') ? R"([\w\d_]+)" : std::string(1, s);
//        }
//        regex_select += "$";
//      }
//      else { // Literal variable name -> only one selected
//        regex_select = "^" + selection + "$";
//      }
//    }
//    }
//    // Loop over branches, append if regex matches
//    std::cout << regex_select << '\n';
//    std::regex re(regex_select);
//
//    for(const auto& leaf : *leaf_list){
//    std::smatch match;
//    std::cout << leaf->GetName() << '\n';
//    std::regex_search(std::string(leaf->GetName()), match, re);
//    //if(match.ready()) std::cout << leaf->GetName() << '\n';
//    }
//    */
//    for(const auto& leaf : *leaf_list){
//        selected.push_back(static_cast<TLeaf*>(leaf));
//    }
//}
//
