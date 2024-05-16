This is my rendition of the LLVM Kaleidoscope language in C++ to familiarize myself with the LLVM. <br>

For anyone who wishes to clone this repository and use the language... <br>
    1. If you intend to build LLVM specifically for this project do the following (Note that full build takes ~20 GB of disk space so I recommend option 2) <br>
    => git clone --recursive https://github.com/dgunther2001/LLVM_Kaleidoscope_Language <br>
    => cd external_libs/llvm-project/llvm <br>
    => mkdir build <br>
    => cd build <br>
    => cmake .. <br>
    => make <br>
    (Note that you can configure for your specific architecture so that the build is smaller) <br>
    => cd this/project/root <br>
    2. If you have an installation of llvm, follow these instructions to save large amounts of disk space and avoid redundant installation <br>
    => install llvm locally using Homebrew on mac, or clone from github, etc... <br>
    => git clone https://github.com/dgunther2001/LLVM_Kaleidoscope_Language <br>
    (Then you just need a path to it, and you can use the following instructions) <br>

Running instructions: <br>
    1. Create a build folder one layer below the root directory <br>
    => mkdir build <br>
    => cd build <br>
    2. Run Cmake files to initialize build in the build folder <br>
    => cmake -DLLVM_DIR= path/to/llvm <br>
    3. Build the entire project <br>
    => make
    4. Run some Kaleidoscope (with some of my own added spice)! <br>
        a. Run without a script directly from the command line <br>
        => ./main
        b. Run a test script (for example: mandelbrot.k in the test folder creates a great visualization of the mandelbrot set) <br>
        => ./main ../tests/mandelbrot.k <br>
        (Or just a path to a file containing Kaleidoscope code that you create!) <br>
