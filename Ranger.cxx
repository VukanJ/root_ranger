#include "Riostream.h"
#include "Ranger.h"

ClassImp(Ranger);

Ranger::Ranger(const TString& rootfile)
  : input_filename(rootfile)
{
  distr = std::uniform_int_distribution<std::mt19937::result_type>(0, ULONG_MAX);
  TTree::SetMaxTreeSize(1000000000000);
  setInputFile(input_filename);
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
  leaf_buffers_B.first.clear();
  leaf_buffers_b.first.clear();
  leaf_buffers_S.first.clear();
  leaf_buffers_s.first.clear();
  leaf_buffers_I.first.clear();
  leaf_buffers_i.first.clear();
  leaf_buffers_F.first.clear();
  leaf_buffers_D.first.clear();
  leaf_buffers_L.first.clear();
  leaf_buffers_l.first.clear();

  leaf_buffers_B.second.clear();
  leaf_buffers_b.second.clear();
  leaf_buffers_S.second.clear();
  leaf_buffers_s.second.clear();
  leaf_buffers_I.second.clear();
  leaf_buffers_i.second.clear();
  leaf_buffers_F.second.clear();
  leaf_buffers_D.second.clear();
  leaf_buffers_L.second.clear();
  leaf_buffers_l.second.clear();
}


void Ranger::setInputFile(const TString& rootfile)
{
  input_filename = rootfile;

  auto inFile = FilePtr(TFile::Open(TString(rootfile), "READ"));

  // Check whether root file is healthy
  if (!inFile->IsOpen()) {
    std::cerr << "\033[91m[ERROR]\033[0m Cannot open file " + input_filename << '\n';
    exit(1);
  }
  if (inFile->IsZombie()) {
    std::cerr << "\033[91m[ERROR]\033[0m Root file appears to be damaged. giving up!\n";
    exit(1);
  }
  clearBuffers();
  formula_buffer.clear();
  inFile->Close();
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
  tree_jobs.push_back({
      {{"formula",     formula},
       {"branch_name", name}},
       Action::add_formula});
}


void Ranger::dev()
{
    TFormula f("F", "[0]**2+[1]**2");
    f.Print();
}


void Ranger::JobValidityCheck(const TreeJob& job)
{
  // Check whether input path and tree exists in source file
  // Prevents confusing segmentation violation if tree is not found
  auto inFile = FilePtr(TFile::Open(input_filename, "READ"));

  if (job.opt.find("tree_in") != job.opt.end()) {
    auto sep = job["tree_in"].rfind('/');
    if (sep != std::string::npos) {
      // Directory + TTree, for example DTT.root:/FolderDir/DecayTree
      TString dir  = job["tree_in"].substr(0, sep);
      TString tree = job["tree_in"].substr(sep + 1);

      auto treedir = inFile->GetDirectory(dir);
      if (treedir == nullptr) {
        std::cerr << "\033[91m[ERROR]\033[0m TDirectory \"" << dir << "\" not found in " << input_filename << '\n';
        exit(1);
      }
      if (treedir->FindKey(tree) == nullptr) {
        std::cerr << "\033[91m[ERROR]\033[0m TTree \"" << tree << "\" not found in " << input_filename << '\n';
        exit(1);
      }
    }
    else if (inFile->FindKey(job("tree_in")) == nullptr) {
      std::cerr << "\033[91m[ERROR]\033[0m TTree \"" << job("tree_in") << "\" not found in " << input_filename << '\n';
      exit(1);
    }
  }
  inFile->Close();
}


void Ranger::Run(TString output_filename)
{
  // Runs all previously defined jobs in sequence (tree-wise)
  outfile_name = output_filename;
  // Create output file
  if (!outfile_name.EndsWith(".root")) {
    outfile_name += ".root";
  }

  // Touch temporary file
  mtgen.seed(std::random_device()());

  temporary_file_name = std::to_string(distr(mtgen)) + '_'
                            + std::to_string(time(0)) + outfile_name;

  auto temporary_file = FilePtr(TFile::Open(temporary_file_name, "RECREATE"));
  temporary_file->Close();

  // Loop over tree jobs
  for (auto& tree_job : tree_jobs) {

    JobValidityCheck(tree_job);

    switch (tree_job.action) {
      case Action::copytree:      SimpleCopy(tree_job);      break;
      case Action::flatten_tree:  flattenTree(tree_job);     break;
      case Action::bpv_selection: BestPVSelection(tree_job); break;
      case Action::add_formula:
           formula_buffer.push_back(std::make_pair(tree_job["branch_name"],
                                                   tree_job["formula"])); break;
      default: break;
    }
    clearBuffers();
  }
  // Delete temporary file from disk
  remove(temporary_file_name);
}


void Ranger::reset()
{
  // Resets jobs and buffers
  tree_jobs.clear();
  formula_buffer.clear();
  clearBuffers();
}


void Ranger::AddBranchesAndCuts(const TreeJob& tree_job, TTree* temp_tree, bool skipcut)
{
  // Add formula branches, apply cuts, write to file, Create final tree
  auto outFile = FilePtr(TFile::Open(outfile_name, "UPDATE"));
  TTree* write_tree = nullptr;

  if (!formula_buffer.empty()) {
      for (const auto& formula : formula_buffer){
        addFormulaBranch(temp_tree, formula.first, formula.second);
      }
    formula_buffer.clear();
  }

  if (!tree_job["cut"].empty() && !skipcut) {
    write_tree = temp_tree->CopyTree(tree_job("cut"));
    write_tree->SetName(tree_job("tree_out"));
  }
  else {
    write_tree = temp_tree->CloneTree();
    write_tree->SetName(tree_job("tree_out"));
  }
  outFile->Delete(TString(temp_tree->GetName()) + ";*");
  outFile->Write("", TObject::kOverwrite);
  outFile->Close();
}


void Ranger::SimpleCopy(const TreeJob& tree_job)
{
    // Copy tree with cut selection and branch selection using built-in methods
    std::cout << "Copying tree " << tree_job["tree_in"] << '\n';
    auto inFile = FilePtr(TFile::Open(input_filename, "READ"));
    TTree* input_tree = static_cast<TTree*>(inFile->Get(tree_job("tree_in")));

    auto outFile = FilePtr(TFile::Open(outfile_name, "UPDATE"));
    outFile->cd();

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
    TTree* output_tree = nullptr;
    if (!tree_job["cut"].empty()) {
      output_tree = input_tree->CopyTree(tree_job("cut"));
    }
    else {
      output_tree = input_tree->CloneTree();
    }

    if (!formula_buffer.empty()) {
      for (const auto& formula : formula_buffer){
        addFormulaBranch(output_tree, formula.first, formula.second);
      }
      formula_buffer.clear();
    }

    output_tree->SetName(tree_job("tree_out"));
    output_tree->SetTitle("root_ranger_tree");
    outFile->Delete(TString(input_tree->GetName()) + ";*");
    outFile->Write("", TObject::kOverwrite);
    outFile->Close();
    inFile->Close();
}


void Ranger::BestPVSelection(const TreeJob& tree_job)
{
  // Loop over tree and copy events into output tree.
  // If TLeaf entries are arrays, select first
  std::cout << "BPV selection on " << tree_job["tree_in"] << '\n';

  auto inputFile = FilePtr(TFile::Open(input_filename, "READ"));
  TTree* input_tree = static_cast<TTree*>(inputFile->Get(tree_job("tree_in")));

  auto temporary_file = FilePtr(TFile::Open(temporary_file_name, "UPDATE"));
  TTree output_tree(tree_job("tree_out") + "_ROOTRANGER_BPV", "root_ranger_tree");
  //keep_trees.push_back(output_tree->GetName());

  input_tree->SetBranchStatus("*", 0);

  std::vector<TLeaf*> all_leaves, bpv_leaves;

  getListOfBranchesBySelection(all_leaves, input_tree, tree_job["branch_selection"]);
  getListOfBranchesBySelection(bpv_leaves, input_tree, tree_job["bpv_branch_selection"]);

  analyzeLeaves_FillLeafBuffers(input_tree, &output_tree, all_leaves, bpv_leaves);

  auto n_entries = input_tree->GetEntriesFast();

  // Event loop
  for (auto event = 0; event < n_entries; ++event) {
    input_tree->GetEntry(event);
    output_tree.Fill();
  }
  temporary_file->Write("", TObject::kOverwrite);
  AddBranchesAndCuts(tree_job, &output_tree);
  temporary_file->Close();
  inputFile->Close();
}


void Ranger::flattenTree(const TreeJob& tree_job)
{
    auto inFile = FilePtr(TFile::Open(input_filename, "READ"));
    TTree* input_tree = static_cast<TTree*>(inFile->Get(tree_job("tree_in")));

    auto temporary_file = FilePtr(TFile::Open(temporary_file_name, "UPDATE"));
    TTree output_tree(tree_job("tree_out") + "_ROOTRANGER_FLAT", "root_ranger_tree");

    input_tree->SetBranchStatus("*", 0);

    std::cout << "Flattening tree " << tree_job["tree_in"] << '\n';

    std::vector<TLeaf*> all_leaves, flat_leaves;

    getListOfBranchesBySelection(all_leaves, input_tree,  tree_job["branch_selection"]);
    getListOfBranchesBySelection(flat_leaves, input_tree, tree_job["flat_branch_selection"]);

    TLeaf* array_length_leaf = analyzeLeaves_FillLeafBuffers(input_tree, &output_tree, all_leaves, flat_leaves);

    // array_length represents array dimension of each element
    int max_array_length = -1;
    input_tree->SetBranchStatus(array_length_leaf->GetName(), 1);
    input_tree->SetBranchAddress(array_length_leaf->GetName(), &max_array_length);
    // Create new branch containing the current array index of each event
    int array_elem_it = -1;
    output_tree.Branch("array_length", &array_elem_it, "array_length/i");

    int n_entries = input_tree->GetEntriesFast();
    // Event loop
    for (int event = 0; event < n_entries; ++event) {
        input_tree->GetEntry(event);
        array_elem_it = 0;
        //std::cout << "EVENT " << event << ' ' << max_array_length << '\n';
        output_tree.Fill();
        for (array_elem_it = 1; array_elem_it < max_array_length; ++array_elem_it) {
            // go to next array element
            incrementBuffer<   Char_t>(array_elem_it);
            incrementBuffer<  UChar_t>(array_elem_it);
            incrementBuffer<  Short_t>(array_elem_it);
            incrementBuffer< UShort_t>(array_elem_it);
            incrementBuffer<    Int_t>(array_elem_it);
            incrementBuffer<   UInt_t>(array_elem_it);
            incrementBuffer< Double_t>(array_elem_it);
            incrementBuffer<  Float_t>(array_elem_it);
            incrementBuffer< Long64_t>(array_elem_it);
            incrementBuffer<ULong64_t>(array_elem_it);
            output_tree.Fill();
        }
    }
    temporary_file->Write("", TObject::kOverwrite);
    AddBranchesAndCuts(tree_job, &output_tree);
    temporary_file->Close();
    inFile->Close();
}


TLeaf* Ranger::analyzeLeaves_FillLeafBuffers(TTree* input_tree, TTree* output_tree,
                                             std::vector<TLeaf*>& all_leaves,
                                             std::vector<TLeaf*>& sel_leaves)
{
  // Analyzes the selected leaves and finds out their dimensionality
  // Multidimensional leaves are assigned more buffer space according
  // to maximum value in array_length leaf that is returned by leaf->GetLeafCount()
  // Returns pointer to array length leaf

  std::map<TLeaf*, std::pair<size_t, bool>> array_length_leaves; // ... and corresponding buffer sizes and whether to flatten them

  for (const auto& leaf : all_leaves) {
    TString LeafName = leaf->GetName();
    TString LeafNameAfter = LeafName;

    bool assign_bufferindex = false;
    size_t buffer_size = 1;

    // Find out leaf dimension
    Int_t probe;
    TLeaf* dim_leaf = leaf->GetLeafCounter(probe);

    if (dim_leaf == nullptr) {
      if (probe > 1) {
        // Leaf elements are arrays / matrices of constant length > 1
        // found_const_array = true;
      }
        // else probe = 1 -> scalar
        buffer_size = probe;
    }
    else {
      // Leaf elements are arrays / matrices of variable length

      // Get max buffer size if unknown
      if (array_length_leaves.find(dim_leaf) == array_length_leaves.end()) {
        input_tree->SetBranchStatus(dim_leaf->GetName(), 1); // !
        array_length_leaves[dim_leaf] = std::make_pair(input_tree->GetMaximum(dim_leaf->GetName()), false);
      }
      if (contains(sel_leaves, leaf)) {
        // Mark leaf for flattening / bpv selection
        LeafNameAfter += "_flat";
        assign_bufferindex = true;
        array_length_leaves[dim_leaf].second = true; // Use for alignment
      }
      buffer_size = array_length_leaves[dim_leaf].first;
    }

    input_tree->SetBranchStatus(LeafName, 1);

    switch (LeafTypeFromStr.find(leaf->GetTypeName())->second) {
      case leaf_char:    addLeaf<   Char_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_uchar:   addLeaf<  UChar_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_short:   addLeaf<  Short_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_ushort:  addLeaf< UShort_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_int:     addLeaf<    Int_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_uint:    addLeaf<   UInt_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_float:   addLeaf<  Float_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_double:  addLeaf< Double_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_long64:  addLeaf< Long64_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
      case leaf_ulong64: addLeaf<ULong64_t>(leaf, LeafNameAfter, input_tree, output_tree, buffer_size, assign_bufferindex); break;
    }
  }

  if (array_length_leaves.size() > 1) {
    TLeaf* alignLeaf = nullptr;
    std::cout << "\033[93m[WARNING]\033[0m More than one array length leaf found:\n";
    for (auto& arl : array_length_leaves) {
      if(arl.second.second) {
        alignLeaf = arl.first;
        std::cout << alignLeaf->GetName() << '\n';
      }
    }
    std::cout << "\033[93m[WARNING]\033[0m Using " << alignLeaf->GetName() << " leaf for alignment. Make sure this is intended\n";
    return alignLeaf;
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


void Ranger::addFormulaBranch(TTree* output_tree, const std::string& name, std::string formula)
{
  // Extract variable strings from formula string
  std::cout << "Compiling formula \"" << formula << "\" --> ";

  std::set<std::string> variables;
  std::regex var_search(R"(\#[\w_][\w\d_]*)"); // Matches variables
  std::sregex_iterator iter(formula.begin(), formula.end(), var_search);
  std::sregex_iterator end;
  for (; iter != end; ++iter) {
    variables.insert(iter->str());
  }

  std::vector<Double_t> buffer(variables.size());

  Double_t result;
  TBranch* formula_branch = output_tree->Branch(TString(name), &result);

  int idx = 0;
  for (std::string var : variables) {
    formula = std::regex_replace(formula, std::regex(var), std::string("[") + std::to_string(idx) + "]");
    var.erase(var.begin()); // Remove '#'
    output_tree->SetBranchStatus(TString(var), 1);
    output_tree->SetBranchAddress(TString(var), &buffer[idx]);
    ++idx;
  }

  std::cout << '\"' << formula << "\"\n";

  TFormula tformula("F", TString(formula));

  int n_entries = output_tree->GetEntriesFast();
  for (int event = 0; event < n_entries; ++event) {
    output_tree->GetEntry(event);
    result = tformula.EvalPar(nullptr, &buffer[0]);

    formula_branch->Fill();
  }
}
