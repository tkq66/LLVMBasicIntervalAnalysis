; ModuleID = 'resources/example1.c'
source_filename = "resources/example1.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %x = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  store i32 5, i32* %b, align 4
  %0 = load i32, i32* %a, align 4
  %cmp = icmp sgt i32 %0, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %1 = load i32, i32* %b, align 4
  %add = add nsw i32 3, %1
  store i32 %add, i32* %x, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %2 = load i32, i32* %b, align 4
  %sub = sub nsw i32 3, %2
  store i32 %sub, i32* %x, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %3 = load i32, i32* %a, align 4
  %cmp1 = icmp sgt i32 %3, 10
  br i1 %cmp1, label %if.then2, label %if.else4

if.then2:                                         ; preds = %if.end
  %4 = load i32, i32* %x, align 4
  %add3 = add nsw i32 3, %4
  store i32 %add3, i32* %x, align 4
  br label %if.end6

if.else4:                                         ; preds = %if.end
  %5 = load i32, i32* %x, align 4
  %sub5 = sub nsw i32 3, %5
  store i32 %sub5, i32* %x, align 4
  br label %if.end6

if.end6:                                          ; preds = %if.else4, %if.then2
  %6 = load i32, i32* %x, align 4
  ret i32 %6
}

attributes #0 = { noinline nounwind optnone ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{!"clang version 7.0.0 (trunk 325693) (llvm/trunk 325690)"}
