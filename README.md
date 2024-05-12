This is my rendition of the LLVM Kaleidoscope language in C++ to familiarize myself with the LLVM.
For anyone who wishes to clone this repository, please do so recursively so that you include the full LLVM project as well
=> git clone --recursive https://github.com/dgunther2001/LLVM_Kaleidoscope_Language

Steps to use project if recursively cloned the repository:
    1. Clone project into a directory on your machine
    2. Options
        a. If LLVM is installed locally on your machine already, run the project from the root directory using cmake -DLLVM_DIR= path/to/llvm
        b. If you want to use the LLVM submodule and build llvm for this project specifically (Note that full build takes ~20 GB of disk space so I recommend option a), cd into external_libs/llvm-project/llvm and create a directory called build. cd into the new build directory and run the command cmake ..