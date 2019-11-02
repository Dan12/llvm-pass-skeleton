cd build
make
cd ..
/usr/local/opt/llvm/bin/opt -instnamer -load build/skeleton/libSkeletonPass.* -skeleton -S test.ll -o test_pass.ll