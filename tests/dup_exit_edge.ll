; ModuleID = 'dup_exit_edge.ll'
source_filename = "tests/dup_exit_edge.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @somefunc(i32 %arg) #0 {
bb:
  %tmp = alloca i32, align 4
  %tmp1 = alloca i32, align 4
  %tmp2 = alloca i32, align 4
  %tmp3 = alloca i32, align 4
  store i32 %arg, i32* %tmp, align 4
  store i32 0, i32* %tmp1, align 4
  store i32 0, i32* %tmp2, align 4
  store i32 0, i32* %tmp3, align 4
  store i32 0, i32* %tmp1, align 4
  br label %bb4

bb4:                                              ; preds = %bb23, %bb
  %tmp24 = load i32, i32* %tmp1, align 4
  %tmp25 = add nsw i32 %tmp24, 1
  store i32 %tmp25, i32* %tmp1, align 4
  %tmp5 = load i32, i32* %tmp1, align 4
  %tmp6 = load i32, i32* %tmp, align 4
  %tmp7 = icmp slt i32 %tmp5, %tmp6
  br i1 %tmp7, label %bb8, label %bb26

bb8:                                              ; preds = %bb4
  store i32 0, i32* %tmp2, align 4
  br label %bb9

bb9:                                              ; preds = %bb19, %bb8
  %tmp10 = load i32, i32* %tmp2, align 4
  %tmp11 = load i32, i32* %tmp1, align 4
  %tmp12 = icmp slt i32 %tmp10, %tmp11
  br label %bb13

bb13:                                             ; preds = %bb9
  %tmp14 = load i32, i32* %tmp1, align 4
  %tmp15 = load i32, i32* %tmp2, align 4
  %tmp16 = mul nsw i32 %tmp14, %tmp15
  %tmp17 = load i32, i32* %tmp3, align 4
  %tmp18 = add nsw i32 %tmp17, %tmp16
  store i32 %tmp18, i32* %tmp3, align 4
  br label %bb19

bb19:                                             ; preds = %bb13
  %tmp20 = load i32, i32* %tmp2, align 4
  %tmp21 = add nsw i32 %tmp20, 1
  store i32 %tmp21, i32* %tmp2, align 4
  br i1 %tmp12, label %bb9, label %bb4

bb26:                                             ; preds = %bb4
  %tmp27 = load i32, i32* %tmp3, align 4
  ret i32 %tmp27
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
