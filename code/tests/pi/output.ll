; ModuleID = 'mini-c'
source_filename = "mini-c"

define float @pi() {
funcBlock:
  %flag = alloca i1
  store i1 false, i1* %flag
  %PI = alloca float
  store float 0.000000e+00, float* %PI
  %i = alloca i32
  store i32 0, i32* %i
  store i1 true, i1* %flag
  store float 3.000000e+00, float* %PI
  store i32 2, i32* %i
  br label %whileFirst

whileFirst:                                       ; preds = %cont, %funcBlock
  %i1 = load i32, i32* %i
  %0 = icmp slt i32 %i1, 100
  %whilecond = icmp ne i1 %0, false
  br i1 %whilecond, label %whileLoop, label %whileEnd

whileLoop:                                        ; preds = %whileFirst
  %flag2 = load i1, i1* %flag
  %ifcond = icmp ne i1 %flag2, false
  br i1 %ifcond, label %if, label %else

if:                                               ; preds = %whileLoop
  %PI3 = load float, float* %PI
  %i4 = load i32, i32* %i
  %i5 = load i32, i32* %i
  %1 = add i32 %i5, 1
  %2 = mul i32 %i4, %1
  %i6 = load i32, i32* %i
  %3 = add i32 %i6, 2
  %4 = mul i32 %2, %3
  %5 = sitofp i32 %4 to float
  %6 = fdiv float 4.000000e+00, %5
  %7 = fadd float %PI3, %6
  store float %7, float* %PI
  br label %cont

else:                                             ; preds = %whileLoop
  %PI7 = load float, float* %PI
  %i8 = load i32, i32* %i
  %i9 = load i32, i32* %i
  %8 = add i32 %i9, 1
  %9 = mul i32 %i8, %8
  %i10 = load i32, i32* %i
  %10 = add i32 %i10, 2
  %11 = mul i32 %9, %10
  %12 = sitofp i32 %11 to float
  %13 = fdiv float 4.000000e+00, %12
  %14 = fsub float %PI7, %13
  store float %14, float* %PI
  br label %cont

cont:                                             ; preds = %else, %if
  %flag11 = load i1, i1* %flag
  %flag12 = load i1, i1* %flag
  %negcmp = icmp eq i1 %flag12, false
  store i1 %negcmp, i1* %flag
  %i13 = load i32, i32* %i
  %15 = add i32 %i13, 2
  store i32 %15, i32* %i
  br label %whileFirst

whileEnd:                                         ; preds = %whileFirst
  %PI14 = load float, float* %PI
  ret float %PI14
}
