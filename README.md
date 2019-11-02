# llvm-pass-skeleton

A completely useless LLVM pass.

Build:

    $ cd llvm-pass-skeleton
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

Run:

    $ clang -S -emit-llvm foo.c
    $  /usr/local/opt/llvm/bin/opt -load build/skeleton/libSkeletonPass.* -skeleton -S foo.ll
