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
                         branch_selection,
                         "",
                         cut_selection,
                         Action::copytree});
}

void LumberJack::flattenTree(cTString& treename,
                             const std::string& branches_flat_selection,
                             const std::string& additional_branches_selection,
                             const std::string& cut_selection,
                             cTString& rename)
{
    tree_jobs.push_back({treename,
                         rename == "" ? treename : rename,
                         branches_flat_selection,
                         additional_branches_selection,
                         cut_selection,
                         Action::flatten_tree});
}

void LumberJack::BPVselection(cTString& treename,
                              const std::string& branches_bpv_selection,
                              const std::string& additional_branches_selection,
                              const std::string& cut_selection,
                              cTString& rename)
{
    tree_jobs.push_back({treename,
                         rename == "" ? treename : rename,
                         branches_bpv_selection,
                         additional_branches_selection,
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
        case Action::bpv_selection: BestPVSelection(tree_job); break;
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
        //output_tree = input_tree->CloneTree(-1, "fast"); // must be tested
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
    // NOT READY YET
    TTree* input_tree = static_cast<TTree*>(inFile->Get(job.name));
    int n_entries = input_tree->GetEntriesFast();

    TTree output_tree(job.newname, job.newname);

    std::vector<TLeaf*> array_leaves;
    getListOfBranchesBySelection(array_leaves, input_tree, job.branch_selection);

    analyzeLeaves_FillLeafBuffers(input_tree, &output_tree, array_leaves);

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
    TTree* input_tree = static_cast<TTree*>(inFile->Get(tree_job.name));
    TTree  output_tree(tree_job.newname, tree_job.newname);

    input_tree->SetBranchStatus("*", 0);

    std::cout << "BPV selection on " << tree_job.name << '\n';

    std::vector<TLeaf*> bpv_leaves, additional_leaves;

    getListOfBranchesBySelection(bpv_leaves, input_tree, tree_job.branch_selection);
    //getListOfBranchesBySelection(additional_leaves, input_tree, tree_job.branch_selection2);

    analyzeLeaves_FillLeafBuffers(input_tree, &output_tree, bpv_leaves);
    //analyzeLeaves_FillLeafBuffers(input_tree, &output_tree, additional_leaves);

    int n_entries = input_tree->GetEntriesFast();

    for(int event = 0; event < 10; ++event){
        input_tree->GetEntry(event);
        output_tree.Fill();
    }
    output_tree.Write();
}

void LumberJack::analyzeLeaves_FillLeafBuffers(TTree* input_tree, TTree* output_tree, std::vector<TLeaf*>& selected_leaves)
{
    // Analyzes the selected leaves and finds out their dimensionality
    // Multidimensional leaves are assigned more buffer space according
    // to maximum value in array_length leaf that is returned by leaf->GetLeafCount()

    std::map<TLeaf*, size_t> array_length_leaves; // ... and corresponding buffer sizes

    bool found_const_array = false;

    for (const auto& leaf : selected_leaves) {
        std::string leaf_type = leaf->GetTypeName();

        size_t buffer_size = 1;

        // Find out leaf dimension
        Int_t probe;
        TLeaf* dim_leaf = leaf->GetLeafCounter(probe);

        if (dim_leaf == nullptr) {
            if (probe > 1) {
                // Leaf elements are arrays / matrices of constant length > 1
                found_const_array = true;
            } // else probe = 1 ->scalar
            buffer_size = probe;
        }
        else {
            // Leaf elements are arrays / matrices of variable length

            // Get max buffer size if unknown
            if(array_length_leaves.find(dim_leaf) == array_length_leaves.end()){
                input_tree->SetBranchStatus(dim_leaf->GetName(), 1); // !
                array_length_leaves[dim_leaf] = input_tree->GetMaximum(dim_leaf->GetName());;
            }
            buffer_size = array_length_leaves[dim_leaf];
        }

        input_tree->SetBranchStatus(leaf->GetName(), 1);

        if(leaf_type == "Float_t"){
            float_leaves.emplace_back(LeafStore<Float_t>(leaf));
            output_tree->Branch(leaf->GetName(), &(float_leaves.back().buffer[0]), leaf->GetTitle(), buffer_size);
        }
        else if(leaf_type == "Double_t"){
            double_leaves.emplace_back(LeafStore<Double_t>(leaf));
            output_tree->Branch(leaf->GetName(), &(double_leaves.back().buffer[0]), leaf->GetTitle(), buffer_size);
        }
        else if(leaf_type == "Int_t"){
            int_leaves.emplace_back(LeafStore<Int_t>(leaf));
            output_tree->Branch(leaf->GetName(), &(int_leaves.back().buffer[0]), leaf->GetTitle(), buffer_size);
        }
    }

    if(array_length_leaves.size() > 1){
        std::cout << "More than one array length leaf found:\n";
        for(auto& arl : array_length_leaves){
            std::cout << arl.first->GetName() << '\n';
        }
        if(found_const_array){
            std::cout << "Alignment leaves and constant array found. Will not select const arrays\n";
        }
        std::cout << "Testing alignment... (not implemented)\n";
    }
}

void LumberJack::getListOfBranchesBySelection(std::vector<TLeaf*>& selected, TTree* target_tree, std::string selection)
{
    // Collects leaves that match regex

    TObjArray* leaf_list = target_tree->GetListOfLeaves();

    std::string regex_select;

    // Remove whitespace
    for(auto c = selection.begin(); c != selection.end();){
        c = (*c == ' ') ? selection.erase(c) : c + 1;
    }
    // Build regex
    if (selection.empty()){
        regex_select = R"([\w\d_]+)";
    }
    else {
        if (selection.size() >= 2){
            if(*selection.begin() == '(' && selection.back() == ')'){
                // User entered regex
                regex_select = selection;
            }
        }
        if (regex_select == "" && contains(selection, '*')){
            // Not a regex. Selected vars by wildcard -> construct regex
            regex_select += "^";
            for(const auto& s : selection){
                regex_select += (s == '*') ? R"([\w\d_]+)" : std::string(1, s);
            }
            regex_select += "$";
        }
        else {
            // Literal variable name -> only one varieble selected
            regex_select = "^" + selection + "$";
        }
    }
    // Loop over branches, append if regex matches
    std::cout << regex_select << '\n';
    std::regex re(regex_select);

    for(const auto& leaf : *leaf_list){
        std::smatch match;
        std::string leafName = std::string(leaf->GetName()); // required for std::regex_search
        if(std::regex_match(leafName, re)){
            selected.push_back(static_cast<TLeaf*>(leaf));
        }
    }

}
