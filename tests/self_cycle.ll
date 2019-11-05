; ModuleID = 'self_cycle_base.ll'
source_filename = "tests/self_cycle_base.c"
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

bb4:                                              ; preds = %bb14, %bb
  %tmp9 = load i32, i32* %tmp, align 4
  %tmp10 = load i32, i32* %tmp1, align 4
  %tmp11 = sdiv i32 %tmp9, %tmp10
  %tmp12 = load i32, i32* %tmp3, align 4
  %tmp13 = add nsw i32 %tmp12, %tmp11
  store i32 %tmp13, i32* %tmp3, align 4
  %tmp15 = load i32, i32* %tmp1, align 4
  %tmp16 = add nsw i32 %tmp15, 1
  store i32 %tmp16, i32* %tmp1, align 4
  %tmp5 = load i32, i32* %tmp1, align 4
  %tmp6 = load i32, i32* %tmp, align 4
  %tmp7 = icmp slt i32 %tmp5, %tmp6
  br i1 %tmp7, label %bb4, label %bb17

bb17:                                             ; preds = %bb4
  store i32 0, i32* %tmp2, align 4
  br label %bb18

bb18:                                             ; preds = %bb28, %bb17
  %tmp23 = load i32, i32* %tmp, align 4
  %tmp24 = load i32, i32* %tmp2, align 4
  %tmp25 = mul nsw i32 %tmp23, %tmp24
  %tmp26 = load i32, i32* %tmp3, align 4
  %tmp27 = add nsw i32 %tmp26, %tmp25
  store i32 %tmp27, i32* %tmp3, align 4
  %tmp29 = load i32, i32* %tmp2, align 4
  %tmp30 = add nsw i32 %tmp29, 2
  store i32 %tmp30, i32* %tmp2, align 4
  %tmp19 = load i32, i32* %tmp2, align 4
  %tmp20 = load i32, i32* %tmp, align 4
  %tmp21 = icmp slt i32 %tmp19, %tmp20
  br i1 %tmp21, label %bb18, label %bb31

bb31:                                             ; preds = %bb18
  %tmp32 = load i32, i32* %tmp3, align 4
  ret i32 %tmp32
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
