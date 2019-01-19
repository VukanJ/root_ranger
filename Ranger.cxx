#include "Riostream.h"
#include "Ranger.h"

ClassImp(Ranger);

Ranger::Ranger(const TString& rootfile)
  : input_filename(rootfile)
{
  TTree::SetMaxTreeSize(1000000000000);
  changeFile(input_filename);
}

Ranger::~Ranger()
{
  closeFile(inFile.get());
  closeFile(outFile.get());
}

void Ranger::closeFile(TFile* fileptr)
{
  if (fileptr != nullptr) {
    if (fileptr->IsOpen()) {
      fileptr->Close();
    }
  }
}

void Ranger::clearBuffers()
{
    leaf_buffers_B.clear();
    leaf_buffers_b.clear();
    leaf_buffers_S.clear();
    leaf_buffers_s.clear();
    leaf_buffers_I.clear();
    leaf_buffers_i.clear();
    leaf_buffers_F.clear();
    leaf_buffers_D.clear();
    leaf_buffers_L.clear();
    leaf_buffers_l.clear();
}

void Ranger::changeFile(const std::string& rootfile)
{
    input_filename = rootfile;

    closeFile(inFile.get());

    inFile = FilePtr(TFile::Open(TString(rootfile), "READ"));

    // Check whether root file is healthy
    if (!inFile->IsOpen()) {
      std::cerr << "Error: Cannot open file " + input_filename << '\n';
      exit(1);
    }
    if (inFile->IsZombie()) {
      std::cerr << "Error: Root file appears to be damaged. giving up\n";
      exit(1);
    }
    clearBuffers();
}

void Ranger::TreeCopy(const std::string& tree_in,
                      const std::string& branch_selection,
                      const std::string& cut_selection,
                      const std::string& tree_out)
{
    tree_jobs.push_back({
        {{"tree_in",          tree_in},
         {"tree_out",         tree_out == "" ? tree_in : tree_out},
         {"branch_selection", branch_selection},
         {"cut",              cut_selection}},
        Action::copytree});
}

void Ranger::FlattenTree(const std::string& tree_in,
				         const std::string& branch_selection,
				         const std::string& flat_branch_selection,
				         const std::string& cut_selection,
				         const std::string& tree_out)
{
    tree_jobs.push_back({
        {{"tree_in",               tree_in}, 
         {"tree_out",              tree_out == "" ? tree_in : tree_out},
         {"branch_selection",      branch_selection},
         {"flat_branch_selection", flat_branch_selection},
         {"cut",                   cut_selection}}, 
        Action::flatten_tree});
}

void Ranger::BPVselection(const std::string& tree_in,
					      const std::string& branch_selection,
					      const std::string& bpv_branch_selection,
					      const std::string& cut_selection,
					      const std::string& tree_out)
{
    tree_jobs.push_back({
        {{"tree_in",              tree_in}, 
         {"tree_out",             tree_out == "" ? tree_in : tree_out},
         {"branch_selection",     branch_selection},
         {"bpv_branch_selection", bpv_branch_selection},
         {"cut",                  cut_selection}}, 
        Action::bpv_selection});
}

void Ranger::addFormula(const std::string& name, std::string formula)
{
    if (tree_jobs.empty()) {
        std::cerr << "Error: Need a previous tree job for adding a formula branch! Skipping\n";
        return;
    }
    tree_jobs.push_back({
        {{"tree_out",    tree_jobs.back()["tree_out"]},
         {"formula",     formula},
         {"branch_name", name}}, 
        Action::add_formula});
}

void Ranger::dev()
{

}
void Ranger::Run(TString output_filename)
{
    // Runs all previously defined jobs in sequence (tree-wise)

    // Create output file
    if (!output_filename.EndsWith(".root")) {
        output_filename += ".root";
    }
    closeFile(outFile.get());
    outFile = FilePtr(TFile::Open(output_filename, "RECREATE"));

    for (auto& tree_job : tree_jobs) {
        switch (tree_job.action) {
        case Action::copytree:      SimpleCopy(tree_job);       break;
        case Action::flatten_tree:  flattenTree(tree_job);      break;
        case Action::bpv_selection: BestPVSelection(tree_job);  break;
        case Action::add_formula:   addFormulaBranch(tree_job); break;
        default: break;
        }
        clearBuffers();
    }
    outFile->Close();
}

void Ranger::reset()
{
    // Resets jobs and buffers
    tree_jobs.clear();
    clearBuffers();
}

void Ranger::SimpleCopy(const TreeJob& tree_job)
{
    // Copy tree with cut selection and branch selection using built-in methods
    TTree* input_tree  = static_cast<TTree*>(inFile->Get(tree_job("tree_in")));
    TTree* output_tree = nullptr;

    if (!tree_job["branch_selection"].empty()) {
        input_tree->SetBranchStatus("*", 0);
        std::vector<TLeaf*> activate_leaves;
        getListOfBranchesBySelection(activate_leaves, input_tree, tree_job["branch_selection"]);
        for (const auto& leaf : activate_leaves) {
            input_tree->SetBranchStatus(leaf->GetName(), 1);
        }
    }
    else {
        input_tree->SetBranchStatus("*", 1);
    }

    if (tree_job["cut"] == "") {
        output_tree = static_cast<TTree*>(input_tree->CloneTree());
    }
    else {
        output_tree = static_cast<TTree*>(input_tree->CopyTree(tree_job("cut")));
    }
    output_tree->SetTitle(tree_job("tree_out"));

    output_tree->Write("", TObject::kOverwrite); // Disable Autosave backups
    std::cout << "Done copying\n";
}

void Ranger::flattenTree(const TreeJob& tree_job)
{
    TTree* input_tree = static_cast<TTree*>(inFile->Get(tree_job("tree_in")));
    TTree output_tree(tree_job("tree_out"), tree_job("tree_out"));
    
    input_tree->SetBranchStatus("*", 0);

    std::cout << "Flattening tree " << tree_job["tree_in"] << '\n';

    std::vector<TLeaf*> all_leaves, flat_leaves;

    getListOfBranchesBySelection(all_leaves, input_tree,  tree_job["branch_selection"]);
    getListOfBranchesBySelection(flat_leaves, input_tree, tree_job["flat_branch_selection"]);

    TLeaf* array_length_leaf = analyzeLeaves_FillLeafBuffers(input_tree, &output_tree, all_leaves, flat_leaves);
    
    // array_length represents array dimension of each element
    int array_length = 1;
    input_tree->SetBranchStatus(array_length_leaf->GetName(), 1);
    input_tree->SetBranchAddress(array_length_leaf->GetName(), &array_length);

    // Create new branch containing the current array index of each event
    int array_elem_it = 0;
    output_tree.Branch("array_length", &array_elem_it, "array_length/i");

    int n_entries = input_tree->GetEntriesFast();

    // Event loop
    for (int event = 0; event < n_entries; ++event) {
        input_tree->GetEntry(event);
        // Update all leaves
        output_tree.Fill(); // Element 0

        for (array_elem_it = 1; array_elem_it < array_length; ++array_elem_it) {
            //for (auto& leaf : leaf_buffers){
            //    leaf->increment(arr_elem);
            //}
            output_tree.Fill();
        }
    }
    if (!tree_job["cut"].empty()) {
        //Create intermediate tree for copying with selection
        auto* output_tree_selected = static_cast<TTree*>(output_tree.CopyTree(tree_job("cut")));
        output_tree_selected->Write("", TObject::kOverwrite);
    }
    else {
        output_tree.Write("", TObject::kOverwrite);
    }
}

void Ranger::BestPVSelection(const TreeJob& tree_job)
{
    // Loop over tree and copy events into output tree.
    // If TLeaf entries are arrays, select first
    TTree* input_tree = static_cast<TTree*>(inFile->Get(tree_job("tree_in")));
    TTree output_tree(tree_job("tree_out"), tree_job("tree_out"));

    input_tree->SetBranchStatus("*", 0);

    std::cout << "BPV selection on " << tree_job["tree_in"] << '\n';

    std::vector<TLeaf*> all_leaves, bpv_leaves;

    getListOfBranchesBySelection(all_leaves, input_tree, tree_job["branch_selection"]);
    getListOfBranchesBySelection(bpv_leaves, input_tree, tree_job["bpv_branch_selection"]);

    analyzeLeaves_FillLeafBuffers(input_tree, &output_tree, all_leaves, bpv_leaves);

    int n_entries = input_tree->GetEntriesFast();

    // Event loop
    for (int event = 0; event < n_entries; ++event) {
        input_tree->GetEntry(event);
        output_tree.Fill();
    }
    if (!tree_job["cut"].empty()) {
        //Create intermediate tree for copying with selection
        TTree* output_tree_selected = static_cast<TTree*>(output_tree.CopyTree(tree_job("cut")));
        output_tree_selected->Write("", TObject::kOverwrite);
    }
    else {
      output_tree.Write("", TObject::kOverwrite);
    }
}

TLeaf* Ranger::analyzeLeaves_FillLeafBuffers(TTree* input_tree, TTree* output_tree,
                                             std::vector<TLeaf*>& all_leaves, 
                                             std::vector<TLeaf*>& sel_leaves)
{
    // Analyzes the selected leaves and finds out their dimensionality
    // Multidimensional leaves are assigned more buffer space according
    // to maximum value in array_length leaf that is returned by leaf->GetLeafCount()
    // Returns pointer to array length leaf

    std::map<TLeaf*, size_t> array_length_leaves; // ... and corresponding buffer sizes

    bool found_const_array = false;

    for (const auto& leaf : all_leaves) {
        TString LeafName = leaf->GetName();
        TString LeafNameAfter = LeafName;

        size_t buffer_size = 1;
        bool has_alignment = false;

        // Find out leaf dimension
        Int_t probe;
        TLeaf* dim_leaf = leaf->GetLeafCounter(probe);

        if (dim_leaf == nullptr) {
            if (probe > 1) {
                // Leaf elements are arrays / matrices of constant length > 1
                found_const_array = true;
            }
            // else probe = 1 -> scalar
            buffer_size = probe;
        }
        else {
            // Leaf elements are arrays / matrices of variable length

            // Get max buffer size if unknown
            if (!contains(sel_leaves, leaf)) {
              // Skipping this leaf since its dimension is not aligned with bpv branches
              // Not sure what to do with these. Ignore for now. Maybe write an extra tree for them
              // std::cout << leaf->GetName() << '\n';
              continue;
            }
            else {
              LeafNameAfter += "_flat";
              has_alignment = true;
            }
            if (array_length_leaves.find(dim_leaf) == array_length_leaves.end()) {
                input_tree->SetBranchStatus(dim_leaf->GetName(), 1); // !
                array_length_leaves[dim_leaf] = input_tree->GetMaximum(dim_leaf->GetName());
            }
            buffer_size = array_length_leaves[dim_leaf];
        }

        if (array_length_leaves.find(leaf) != array_length_leaves.end()) {
            // Do not write array leaves to new tree. If flattening write new index branch
            input_tree->SetBranchStatus(leaf->GetName(), 0);
            continue;
        }

        input_tree->SetBranchStatus(LeafName, 1);

        switch (LeafTypeFromStr.find(leaf->GetTypeName())->second) {
        case leaf_char:    addLeaf<   Char_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_uchar:   addLeaf<  UChar_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_short:   addLeaf<  Short_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_ushort:  addLeaf< UShort_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_int:     addLeaf<    Int_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_uint:    addLeaf<   UInt_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_float:   addLeaf<  Float_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_double:  addLeaf< Double_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_long64:  addLeaf< Long64_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
	    case leaf_ulong64: addLeaf<ULong64_t>(LeafName, LeafNameAfter, input_tree, output_tree, buffer_size, has_alignment); break;
        }
    }

    if (array_length_leaves.size() > 1) {
        std::cout << "[WARNING] More than one array length leaf found:\n";
        for (auto& arl : array_length_leaves) {
            std::cout << arl.first->GetName() << '\n';
        }
        std::cout << "[WARNING] Using " << array_length_leaves.begin()->first->GetName() << " leaf for alignment. Make sure this is intended\n";
    }
    return array_length_leaves.begin()->first;
}

void Ranger::getListOfBranchesBySelection(std::vector<TLeaf*>& selected, TTree* target_tree, std::string selection)
{
    // Collects leaves that match regex
    TObjArray* leaf_list = target_tree->GetListOfLeaves();

    std::string regex_select;

    // Remove whitespace
    for (auto c = selection.begin(); c != selection.end();) {
        c = (*c == ' ') ? selection.erase(c) : c + 1;
    }
    // Build regex
    if (selection.empty()) {
        return;
    }
    else {
        if (selection.size() >= 2) {
            if (*selection.begin() == '(' && selection.back() == ')') {
                // User entered regex
                regex_select = selection;
            }
        }
        if (regex_select == "" && contains(selection, '*')) {
            // Not a regex. Selected vars by wildcard -> construct regex
            regex_select += "^";
            for (const auto& s : selection) {
                regex_select += (s == '*') ? R"([\w\d_]+)" : std::string(1, s);
            }
            regex_select += "$";
        }
        else {
            // Literal variable name -> only one variable can be selected
            regex_select = "^" + selection + "$";
        }
    }
    // Loop over branches, append if regex matches name
    std::regex re(regex_select);

    for (const auto& leaf : *leaf_list) {
        std::smatch match;
        std::string leafName = std::string(leaf->GetName()); // required for std::regex_search
        if (std::regex_match(leafName, re)) {
            selected.push_back(static_cast<TLeaf*>(leaf));
        }
    }
}

void Ranger::addFormulaBranch(const TreeJob& tree_job)
{
    TTree* output_tree_existing = static_cast<TTree*>(outFile->Get(tree_job("tree_out")));
    std::string formula = tree_job["formula"]; // Name does not really fit here ...

    //return;
    std::regex var_search(R"(\#[\w_][\w\d_]*)"); // Matches variables

    std::set<std::string> variables;

    // Extract variables from formula string
    std::sregex_iterator iter(formula.begin(), formula.end(), var_search);
    std::sregex_iterator end;
    for (; iter != end; ++iter) {
        variables.insert(iter->str());
    }

    std::vector<Double_t> buffer(variables.size());

    Double_t result;
    TBranch* formula_branch = output_tree_existing->Branch(tree_job("branch_name"), &result);

    int idx = 0;
    for (std::string var : variables) {
        formula = std::regex_replace(formula, std::regex(var), std::string("[") + std::to_string(idx) + "]");
        var.erase(var.begin()); // Remove '#'
        output_tree_existing->SetBranchStatus(TString(var), 1);
        output_tree_existing->SetBranchAddress(TString(var), &buffer[idx]);
        ++idx;
    }

    TFormula tformula("F", TString(formula));

    int n_entries = output_tree_existing->GetEntriesFast();
    for (int event = 0; event < n_entries; ++event) {
        output_tree_existing->GetEntry(event);
        result = tformula.EvalPar(nullptr, &buffer[0]);

        formula_branch->Fill();
    }
    output_tree_existing->Write("", TObject::kOverwrite);
}

