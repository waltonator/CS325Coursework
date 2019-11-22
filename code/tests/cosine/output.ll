; ModuleID = 'mini-c'
source_filename = "mini-c"

declare float @print_float(float)

define float @cosine(float %x) {
funcBlock:
  %x1 = alloca float
  store float %x, float* %x1
  %cos = alloca float
  store float 0.000000e+00, float* %cos
  %n = alloca float
  store float 0.000000e+00, float* %n
  %term = alloca float
  store float 0.000000e+00, float* %term
  %eps = alloca float
  store float 0.000000e+00, float* %eps
  %alt = alloca float
  store float 0.000000e+00, float* %alt
  store float 0x3EB0C6F7A0000000, float* %eps
  store float 1.000000e+00, float* %n
  store float 1.000000e+00, float* %cos
  store float 1.000000e+00, float* %term
  store float -1.000000e+00, float* %alt
  br label %whileFirst

whileFirst:                                       ; preds = %whileLoop, %funcBlock
  %term2 = load float, float* %term
  %eps3 = load float, float* %eps
  %0 = fcmp ugt float %term2, %eps3
  %whilecond = icmp ne i1 %0, false
  br i1 %whilecond, label %whileLoop, label %whileEnd

whileLoop:                                        ; preds = %whileFirst
  %term4 = load float, float* %term
  %x5 = load float, float* %x1
  %1 = fmul float %term4, %x5
  %x6 = load float, float* %x1
  %n7 = load float, float* %n
  %2 = fdiv float %x6, %n7
  %n8 = load float, float* %n
  %3 = fadd float %n8, 1.000000e+00
  %4 = fdiv float %2, %3
  %5 = fmul float %1, %4
  store float %5, float* %term
  %cos9 = load float, float* %cos
  %alt10 = load float, float* %alt
  %term11 = load float, float* %term
  %6 = fmul float %alt10, %term11
  %7 = fadd float %cos9, %6
  store float %7, float* %cos
  %alt12 = load float, float* %alt
  %alt13 = load float, float* %alt
  %8 = fsub float -0.000000e+00, %alt13
  store float %8, float* %alt
  %n14 = load float, float* %n
  %9 = fadd float %n14, 2.000000e+00
  store float %9, float* %n
  br label %whileFirst

whileEnd:                                         ; preds = %whileFirst
  %cos15 = load float, float* %cos
  %10 = call float @print_float(float %cos15)
  %cos16 = load float, float* %cos
  ret float %cos16
}
