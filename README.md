### srcUML
* A tool for efficiently reverse engineering accurate UML class diagrams.

### Cloning
* The project uses srcSAX and srcSAXEventDispatch as submodules.  The repo most be cloned with the `--recursive` option to clone the submodules.  The following is an example command:
```bash
git clone --recursive https://github.com/srcML/srcUML.git
```

### Build
The project utilizes CMake for building.
The following provide examples of how to build.  srcuml will be placed in the ./bin directory in all cases.

#### Dependencies
- libxml2
- Boost
- OGDF

```bash
# in-source build
cmake .
make

# out-of-source build
mkdir srcuml_build
cd srcuml_build
cmake path_to_srcUML_repo
make

# release mode
mkdir srcuml_build
cd srcuml_build
cmake -DCMAKE_BUILD_TYPE=Release path_to_srcUML_repo
make
```

### Note
* The current build is dependent on srcML 1.0 (not yet publicly released).
