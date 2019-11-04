; ModuleID = 'tests/diamond.ll'
source_filename = "tests/diamond.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @somefunc(i32 %arg) #0 {
A:
  %tmp = alloca i32, align 4
  %tmp1 = alloca i32, align 4
  store i32 %arg, i32* %tmp, align 4
  store i32 0, i32* %tmp1, align 4
  %tmp2 = load i32, i32* %tmp, align 4
  %tmp3 = icmp ne i32 %tmp2, 0
  br i1 %tmp3, label %B, label %C

B:                                              ; preds = %bb
  store i32 5, i32* %tmp1, align 4
  %tmp6 = load i32, i32* %tmp, align 4
  %tmp7 = icmp sle i32 %tmp6, 0
  br i1 %tmp7, label %D, label %C

C:                                             ; preds = %bb5
  %tmp12 = load i32, i32* %tmp, align 4
  %tmp13 = mul nsw i32 %tmp12, 2
  %tmp14 = load i32, i32* %tmp1, align 4
  %tmp15 = add nsw i32 %tmp14, %tmp13
  store i32 %tmp15, i32* %tmp1, align 4
  br label %D

D:                                             ; preds = %bb11, %bb8
  %tmp9 = load i32, i32* %tmp1, align 4
  %tmp10 = add nsw i32 %tmp9, -12
  store i32 %tmp10, i32* %tmp1, align 4
  %tmp17 = load i32, i32* %tmp1, align 4
  %tmp18 = mul nsw i32 %tmp17, 3
  store i32 %tmp18, i32* %tmp1, align 4
  %tmp19 = load i32, i32* %tmp, align 4
  %tmp20 = icmp eq i32 %tmp19, 5
  br i1 %tmp20, label %E, label %F

E:                                             ; preds = %bb16
  %tmp22 = load i32, i32* %tmp1, align 4
  %tmp23 = add nsw i32 %tmp22, 4
  store i32 %tmp23, i32* %tmp1, align 4
  br label %F

F:                                             ; preds = %bb21, %bb16
  %tmp25 = load i32, i32* %tmp1, align 4
  %tmp26 = add nsw i32 %tmp25, 4
  ret i32 %tmp26
}

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @main() #0 {
bb:
  %tmp = alloca i32, align 4
  %tmp1 = alloca i32, align 4
  store i32 0, i32* %tmp, align 4
  store i32 0, i32* %tmp1, align 4
  %tmp2 = call i32 @somefunc(i32 10)
  %tmp3 = load i32, i32* %tmp1, align 4
  %tmp4 = add nsw i32 %tmp3, %tmp2
  store i32 %tmp4, i32* %tmp1, align 4
  %tmp5 = call i32 @somefunc(i32 -10)
  %tmp6 = load i32, i32* %tmp1, align 4
  %tmp7 = add nsw i32 %tmp6, %tmp5
  store i32 %tmp7, i32* %tmp1, align 4
  %tmp8 = call i32 @somefunc(i32 0)
  %tmp9 = load i32, i32* %tmp1, align 4
  %tmp10 = add nsw i32 %tmp9, %tmp8
  store i32 %tmp10, i32* %tmp1, align 4
  %tmp11 = call i32 @somefunc(i32 4)
  %tmp12 = load i32, i32* %tmp1, align 4
  %tmp13 = add nsw i32 %tmp12, %tmp11
  store i32 %tmp13, i32* %tmp1, align 4
  %tmp14 = load i32, i32* %tmp1, align 4
  ret i32 %tmp14
}

attributes #0 = { noinline nounwind optnone ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{!"clang version 9.0.0 (tags/RELEASE_900/final)"}
