# **root_ranger**
#### Tool for performing standard operations on root tuples

#### Prerequisites
* A recent version of ROOT compiled using the C++17 standard
* C++17 compiler

#### Installation
```
    make
```
#### Usage
Ranger can be imported into C++ and python scripts. User-friendly python bindings are defined in `root_ranger.py`.
The working principle of Ranger is to define one or multiple "jobs" for each tree that should be applied in sequence and to execute them on one or multiple files.
#### Scope
Ranger is especially useful when working with array variables in root files that were
created by the [DecayTreeFitter](https://twiki.cern.ch/twiki/bin/view/LHCb/DecayTreeFitter)

**Features**
* Copying trees with selections and branch selections
* Flattening of leaves with array dimension
* Best primary vertex selection
* Adding branches using arbitrarily complex formulas
#### Limitations
Ranger does not yet support boolean leaves
## Example 1:
Copy a tree with a selection and rename the tree in the new file
```python
    from root_ranger import Ranger

    ranger = Ranger("DTT.root")
    ranger.copy_tree("DecayTree", cut="BKGCAT==0",  dest="DecayTreeSignal")
    ranger.run("DTT_signal.root")
```
## Example 2:
Copy the tree containing the integrated luminosity, perform a best primary vertex selection (assuming we used *DecayTreeFitter*) on another tree and only keep a subset of branches and apply a selection and repeat this procedure with 10 different files that have a similar structure.
```python
    from root_ranger import Ranger

    ranger = Ranger("DTT_0.root")
    ranger.bpv_selection("DecayTree", branches=["B0_Fit*", "piminus*"],
                         bpv_branches="B0_Fit*",
                         cut=["some_chi2<30", "piminus_PT>1000"])
    ranger.copy_tree("GetIntegratedLuminosity/LumiTuple", dest="LumiTuple")

    ranger.run("DTT_truth_0.root")
    for i in range(1, 10):
        ranger.set_input_file("DTT_{0}.root".format(i))
        ranger.run("DTT_out{0}.root".format(i))
```
Here, we are keeping all branches that start with "B0_Fit" or "piminus". Wildcards can be used anywhere in the selection strings.
## Example 3:
Do a bpv selection and flatten the array dimension of the *same*
tree, but store the results in two different trees in the output file.
```python
    from root_ranger import Ranger

    ranger = Ranger("DTT.root")
    ranger.bpv_selection("DecayTree", bpv_branches="B0_Fit*",  dest="DecayTree_bpv")
    ranger.flatten_tree("DecayTree",  flat_branches="B0_Fit*", dest="DecayTree_flat")

    ranger.run("DTT_out.root")
```
## Example 4:
Apply a certain operation to a tree in a file, but then do something different with another file.
```python
    from root_ranger import Ranger

    ranger = Ranger("DTT.root")
    ranger.bpv_selection("DecayTree", bpv_branches="B0_Fit*")

    ranger.run("DTT_out.root")

    ranger.reset() # Reset all previous job descriptions
    ranger.change_file("DTT_2.root")
    ranger.flatten_tree("DecayTree", flat_branches="B0_Fit*")

    ranger.run("DTT_2_out.root")
```
Alternatively, multiple Ranger instances can be used
## Example 5:
Add the transverse momentum of kaons to the tuple and simultaneously apply to it. Only write a certain list of branches to the new file.
```python
    from root_ranger import Ranger

    keep_branches = [
        "*PT",       # All branches ending with PT
        "*M",        # All branches ending with M
        "KS0_P[XY]", # KS0_PX and KS0_PY
        "Kaon_PT"
    ]

    ranger = Ranger("DTT.root")
    ranger.add_formula("Kaon_PT", "TMath::Sqrt(#KS0_PX**2+#KS0_PY**2)")
    ranger.copy_tree("DecayTree", branches=keep_branches, cut="Kaon_PT>500")

    ranger.run("DTT_out.root")
```
The `add_formula` command adds another formula to a list of formulas
that are added when the next tree is written. Branch names in formulas must start with `'#'`
