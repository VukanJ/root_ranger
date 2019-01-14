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

void LumberJack::treeCopy(cTString& tree, cTString& rename)
{
    tree_jobs.push_back({tree,
                         rename == "" ? tree : rename,
                         "", "", "",
                         Action::copytree});
}

void LumberJack::treeCopySelection(cTString& treename,
                                   const std::string& branch_selection,
                                   const std::string& cut_selection,
                                   cTString& rename)
{
    tree_jobs.push_back({treename,
                         rename == "" ? treename : rename,
                         "",
                         branch_selection,
                         cut_selection,
                         Action::copytree});
}

void LumberJack::flattenTree(cTString& treename,
                             cTString& array_dimension,
                             const std::string& branch_selection,
                             const std::string& cut_selection,
                             cTString& rename)
{
    tree_jobs.push_back({treename,
                         rename == "" ? treename : rename,
                         array_dimension,
                         branch_selection,
                         cut_selection,
                         Action::flatten_tree});
}

void LumberJack::BPVselection(cTString& treename,
                              TString&  array_dimension,
                              const std::string& branch_selection,
                              const std::string& cut_selection,
                              cTString& rename)
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
    TTree* tree = (TTree*)inFile->Get("inclusive_Jpsi/DecayTree");
    std::vector<Float_t> mass;
    std::vector<Float_t> chi2;
    Int_t NPV;
    Float_t P;
    //tree->SetBranchStatus("*", 0);

    mass.reserve(10);
    chi2.reserve(10);

    tree->SetBranchStatus("*", 0);
    tree->SetBranchStatus("B0_FitDaughtersConst_M",    1);
    tree->SetBranchStatus("B0_FitDaughtersConst_nPV",  1);
    tree->SetBranchStatus("B0_FitDaughtersConst_chi2", 1);

    std::cout << tree->GetLeaf("B0_FitDaughtersConst_M")->GetLeafCount()->GetName() << std::endl;

    tree->SetBranchAddress("B0_FitDaughtersConst_M",    &mass[0]);
    tree->SetBranchAddress("B0_FitDaughtersConst_chi2", &chi2[0]);
    tree->SetBranchAddress("B0_FitDaughtersConst_nPV", &NPV);

    std::cout << std::setprecision(10);

    for(int i = 0; i < 100; ++i){
        tree->GetEntry(i);
        std::cout << "NPV = " << NPV << '\n';
        for(int npv = 0; npv < NPV; ++npv) std::cout << "M = " << mass[npv] << ' ';
        std::cout << '\n';
        for(int npv = 0; npv < NPV; ++npv) std::cout << "chi2 = " << chi2[npv] << ' ';
        std::cout << '\n';
    }

}

void LumberJack::Print() const
{
    // Print what LumberJack is about to do if Run() is called
    std::cout << "LumberJack job for " << input_filename << ":\n";
    for(const auto& tree_job : tree_jobs){
        switch(tree_job.action){
        case Action::copytree:
            std::cout << "\t Copy tree:\t" << tree_job.name;
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
        case Action::copytree: SimpleCopy(tree_job); break;
        case Action::flatten_tree: flatten(tree_job); break;
        default: break;
        }
    }
}

void LumberJack::SimpleCopy(const TreeJob& job)
{
    // Copy tree with cut selection and branch selection with built in methods
    TTree* input_tree = static_cast<TTree*>(inFile->Get(job.name));
    TTree* output_tree = nullptr;
    input_tree->SetBranchStatus("*", 1);

    if(job.cut_selection == ""){
        output_tree = input_tree->CloneTree();
    }
    else {
        output_tree = input_tree->CopyTree(TString(job.cut_selection));
    }
    output_tree->SetTitle(job.newname);

    output_tree->Write("", TObject::kOverwrite); // Disable Autosave backups
    delete output_tree;
}

void LumberJack::flatten(const TreeJob& job)
{
    TTree* input_tree = static_cast<TTree*>(inFile->Get(job.name));
    int n_entries = input_tree->GetEntriesFast();

    TTree output_tree(job.newname, job.newname);

    std::vector<TLeaf*> array_leaves;
    getListOfBranchesBySelection(array_leaves, input_tree, job.branch_selection);

    prepareOutputTree(&output_tree, array_leaves);

    for(int evt = 0; evt < n_entries; ++evt){
        input_tree->GetEntry(evt);
        output_tree.Fill();
    }
    output_tree.Write("", TObject::kOverwrite);
}

void LumberJack::BestPVSelection(const TreeJob& tree_job)
{
    // Loop over tree and copy events to output tree.
    // If Leaf entries are arrays, select first
}

void LumberJack::prepareOutputTree(TTree* output_tree, std::vector<TLeaf*>& selected_leaves)
{
    for(const auto& leaf : selected_leaves){
        std::string leaf_type = leaf->GetTypeName();
        if(leaf_type == "Float_t"){
            float_leaves.emplace_back(LeafStore<Float_t>(leaf, FLOAT_LEAF));
            output_tree->Branch(leaf->GetName(), &float_leaves.back(), leaf->GetTitle());

            std::string K;
            bool write = false;
            for(char c : std::string(leaf->GetTitle())){
                if(c == '['){
                    write = true;
                }
                else if(c == ']') break;
                else if (write) K += c;
            }
            std::cout << K << '\n';
        }
        else if(leaf_type == "Double_t"){
            std::cout << leaf->GetTitle() << '\n';
            double_leaves.emplace_back(LeafStore<Double_t>(leaf, DOUBLE_LEAF));
            output_tree->Branch(leaf->GetName(), &double_leaves.back(), leaf->GetTitle());
        }
        else if(leaf_type == "Int_t"){
            int_leaves.emplace_back(LeafStore<Int_t>(leaf, INT_LEAF));
            output_tree->Branch(leaf->GetName(), &int_leaves.back(), leaf->GetTitle());
        }
        else if(leaf_type == "Long_t"){
            long_leaves.emplace_back(LeafStore<Long_t>(leaf, LONG_LEAF));
            output_tree->Branch(leaf->GetName(), &long_leaves.back(), leaf->GetTitle());
        }
        else if(leaf_type == "ULong_t"){
            ulong_leaves.emplace_back(LeafStore<ULong_t>(leaf, ULONG_LEAF));
            output_tree->Branch(leaf->GetName(), &ulong_leaves.back(), leaf->GetTitle());
        }
    }
}

void LumberJack::getListOfBranchesBySelection(std::vector<TLeaf*>& selected, TTree* target_tree, std::string selection)
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
    // Placeholder until I have access to a modern C++ compiler
    for(const auto& leaf : *leaf_list){
        if(TString(leaf->GetName()).BeginsWith(TString(selection))){
            selected.push_back(static_cast<TLeaf*>(leaf));
        }
    }
}
