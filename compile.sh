cd build
make
cd ..
/usr/local/opt/llvm/bin/clang -emit-llvm -S tests/break.c
/usr/local/opt/llvm/bin/opt -load build/skeleton/libSkeletonPass.* -skeleton -S break.ll -o test_pass.ll
/usr/local/opt/llvm/bin/clang -c test_pass.ll
/usr/local/opt/llvm/bin/clang -c extras.c
/usr/local/opt/llvm/bin/clang extras.o test_pass.o -o test.bin
./test.bin
rm *.ll *.o *.bin