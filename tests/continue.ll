; ModuleID = 'continue_base.ll'
source_filename = "tests/continue_base.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @somefunc(i32 %arg) #0 {
bb:
  %tmp = alloca i32, align 4
  %tmp1 = alloca i32, align 4
  %tmp2 = alloca i32, align 4
  store i32 %arg, i32* %tmp, align 4
  store i32 0, i32* %tmp1, align 4
  store i32 0, i32* %tmp2, align 4
  store i32 0, i32* %tmp1, align 4
  br label %bb3

bb3:                                              ; preds = %bb17, %bb
  %tmp4 = load i32, i32* %tmp1, align 4
  %tmp5 = load i32, i32* %tmp, align 4
  %tmp6 = icmp slt i32 %tmp4, %tmp5
  br i1 %tmp6, label %bb7, label %bb20

bb7:                                              ; preds = %bb3
  %tmp8 = load i32, i32* %tmp1, align 4
  %tmp9 = icmp eq i32 %tmp8, 4
  br i1 %tmp9, label %bb3, label %bb13

bb13:                                             ; preds = %bb7
  %tmp14 = load i32, i32* %tmp1, align 4
  %tmp15 = load i32, i32* %tmp2, align 4
  %tmp16 = add nsw i32 %tmp15, %tmp14
  store i32 %tmp16, i32* %tmp2, align 4
  br label %bb17

bb17:                                             ; preds = %bb13, %bb10
  %tmp18 = load i32, i32* %tmp1, align 4
  %tmp19 = add nsw i32 %tmp18, 1
  store i32 %tmp19, i32* %tmp1, align 4
  br label %bb3

bb20:                                             ; preds = %bb3
  %tmp21 = load i32, i32* %tmp2, align 4
  ret i32 %tmp21
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
