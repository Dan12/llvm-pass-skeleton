; ModuleID = 'test_loop.ll'
source_filename = "tests/test_loop.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @loopy(i32) #0 {
block0:
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, i32* %1, align 4
  store i32 0, i32* %3, align 4
  store i32 0, i32* %2, align 4
  br label %block1

block1:                                           ; preds = %block6, %block0
  %4 = load i32, i32* %2, align 4
  %5 = load i32, i32* %1, align 4
  %6 = icmp slt i32 %4, %5
  br i1 %6, label %block2, label %block7

block2:                                           ; preds = %block1
  %7 = load i32, i32* %2, align 4
  %8 = srem i32 %7, 2
  %9 = icmp eq i32 %8, 0
  br i1 %9, label %block3, label %block4

block3:                                           ; preds = %block2
  %10 = load i32, i32* %2, align 4
  %11 = load i32, i32* %3, align 4
  %12 = add nsw i32 %11, %10
  store i32 %12, i32* %3, align 4
  br label %block5

block4:                                           ; preds = %block2
  %13 = load i32, i32* %2, align 4
  %14 = load i32, i32* %3, align 4
  %15 = mul nsw i32 %14, %13
  store i32 %15, i32* %3, align 4
  br label %block5

block5:                                           ; preds = %block4, %block3
  br label %block6

block6:                                           ; preds = %block5
  %16 = load i32, i32* %2, align 4
  %17 = add nsw i32 %16, 1
  store i32 %17, i32* %2, align 4
  br label %block1

block7:                                           ; preds = %block1
  %18 = load i32, i32* %3, align 4
  ret i32 %18
}

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @main() #0 {
block0:
  %0 = alloca i32, align 4
  store i32 0, i32* %0, align 4
  %1 = call i32 @loopy(i32 3)
  %2 = call i32 @loopy(i32 -1)
  %3 = call i32 @loopy(i32 4)
  ret i32 %3
}

attributes #0 = { noinline nounwind optnone ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{!"clang version 9.0.0 (tags/RELEASE_900/final)"}
