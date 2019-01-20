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
* WIP: Random primary vertex selection 
* WIP: Leaf removal
#### Limitations
Ranger does not yet support boolean leaves
## Example 1:
We want to copy a tree with a selection and rename the tree in the new file
```python
from root_ranger import Ranger

ranger = Ranger("DTT.root")
ranger.copy_tree("DecayTree", cut="BKGCAT==0", dest="DecayTreeTruth")
ranger.Run("DTT_truth.root")
```
## Example 2:
We want to copy the tree containing the integrated luminosity, perform a best primary vertex selection (assuming we used *DecayTreeFitter*) on another tree and only keep a subset of branches and apply a selection and repeat this procedure with 10 different files that have a similar structure.
```python
from root_ranger import Ranger

ranger = Ranger("DTT_0.root")
ranger.bpv_selection("DecayTree", branches=["B0_Fit*", "piminus*"], 
                                  bpv_branches="B0_Fit*",
                                  cut="some_chi2<30")
ranger.copy_tree("GetIntegratedLuminosity/LumiTuple", dest="LumiTuple")

ranger.run("DTT_truth_0.root")
for i in range(1, 10):
    ranger.change_file("DTT_{0}.root".format(i))
    ranger.run("DTT_out{0}.root".format(i))
```
Here, we are keeping all branches that start with "B0_Fit" or "piminus". Wildcards can be used anywhere in the selection strings. Alternatively, a regex can be specified by writing it in parantheses, for example `branches='(B0_P[XY])'`
## Example 3:
We want to do a bpv selection and flatten the array dimension of the *same*
tree, but store the results in two different trees in the output file.
```python
from root_ranger import Ranger

ranger = Ranger("DTT.root")
ranger.bpv_selection("DecayTree", bpv_branches="B0_Fit*", dest="DecayTree_bpv")
ranger.flatten_tree("DecayTree", flat_branches="B0_Fit*", dest="DecayTree_flat")

ranger.Run("DTT_out.root")
```
## Example 4:
We want to do a certain operation on one file, but something different for another file
```python
from root_ranger import Ranger

ranger = Ranger("DTT.root")
ranger.bpv_selection("DecayTree", bpv_branches="B0_Fit*")

ranger.Run("DTT_out.root")

ranger.reset() # Reset all previous job descriptions
ranger.changeFile("DTT_2.root")
ranger.flatten_tree("DecayTree", flat_branches="B0_Fit*")

ranger.Run("DTT_2_out.root")
```
Alternatively, multiple Ranger instances can be used
## Example 4:
Adding the transverse momentum of kaons to the tuple and applying a cut on it simultaneously. 
```python
from root_ranger import Ranger

ranger = Ranger("DTT.root")
ranger.add_formula("Kaon_PT", "TMath::Sqrt(#KS0_PX**2+#KS0_PY**2)")
ranger.copy_tree("DecayTree", cut="Kaon_PT>500") 

ranger.Run("DTT_out.root")
```
The `add_formula` command adds another formula to a list of formulas 
that are added when the next tree is written.
