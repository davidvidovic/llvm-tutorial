# llvm-tutorial

Repo for LLVM learning purposes - everything about implementing new custom LLVM IR passes.

Build:

    $ cd llvm-custom-passes/<name-of-folder>
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

Run:

    $ clang -fpass-plugin=`echo build/src/<pass-name>.*` -emit-llvm -S -o - test.c

To see Clang's original LLVM IR output:

	$ clang -O0 -emit-llvm -S -o - test.c
