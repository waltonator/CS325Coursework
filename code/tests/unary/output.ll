; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

declare float @print_float(float)

define float @unary(i32 %n, float %m) {
funcBlock:
  %m2 = alloca float
  %n1 = alloca i32
  store i32 %n, i32* %n1
  store float %m, float* %m2
  %result = alloca float
  store float 0.000000e+00, float* %result
  %sum = alloca float
  store float 0.000000e+00, float* %sum
  store float 0.000000e+00, float* %sum
  %n3 = load i32, i32* %n1
  %m4 = load float, float* %m2
  %0 = sitofp i32 %n3 to float
  %1 = fadd float %0, %m4
  store float %1, float* %result
  %result5 = load float, float* %result
  %2 = call float @print_float(float %result5)
  %sum6 = load float, float* %sum
  %result7 = load float, float* %result
  %3 = fadd float %sum6, %result7
  store float %3, float* %sum
  %n8 = load i32, i32* %n1
  %m9 = load float, float* %m2
  %m10 = load float, float* %m2
  %4 = fsub float -0.000000e+00, %m10
  %5 = sitofp i32 %n8 to float
  %6 = fadd float %5, %4
  store float %6, float* %result
  %result11 = load float, float* %result
  %7 = call float @print_float(float %result11)
  %sum12 = load float, float* %sum
  %result13 = load float, float* %result
  %8 = fadd float %sum12, %result13
  store float %8, float* %sum
  %n14 = load i32, i32* %n1
  %m15 = load float, float* %m2
  %9 = sitofp i32 %n14 to float
  %10 = fadd float %9, %m15
  store float %10, float* %result
  %result16 = load float, float* %result
  %11 = call float @print_float(float %result16)
  %sum17 = load float, float* %sum
  %result18 = load float, float* %result
  %12 = fadd float %sum17, %result18
  store float %12, float* %sum
  %n19 = load i32, i32* %n1
  %n20 = load i32, i32* %n1
  %13 = sub i32 0, %n20
  %m21 = load float, float* %m2
  %m22 = load float, float* %m2
  %14 = fsub float -0.000000e+00, %m22
  %15 = sitofp i32 %13 to float
  %16 = fadd float %15, %14
  store float %16, float* %result
  %result23 = load float, float* %result
  %17 = call float @print_float(float %result23)
  %sum24 = load float, float* %sum
  %result25 = load float, float* %result
  %18 = fadd float %sum24, %result25
  store float %18, float* %sum
  %sum26 = load float, float* %sum
  ret float %sum26
}
