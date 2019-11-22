; ModuleID = 'mini-c'
source_filename = "mini-c"

@test = global i32 12
@f = global float 0.000000e+00
@b = global i1 false

declare i32 @print_int(i32)

define i32 @While(i32 %n) {
funcBlock:
  %n1 = alloca i32
  store i32 %n, i32* %n1
  %result = alloca i32
  store i32 0, i32* %result
  store i32 0, i32* %result
  %0 = call i32 @print_int(i32 12)
  br label %whileFirst

whileFirst:                                       ; preds = %whileLoop, %funcBlock
  %result2 = load i32, i32* %result
  %1 = icmp slt i32 %result2, 10
  %whilecond = icmp ne i1 %1, false
  br i1 %whilecond, label %whileLoop, label %whileEnd

whileLoop:                                        ; preds = %whileFirst
  %result3 = load i32, i32* %result
  %2 = add i32 %result3, 1
  store i32 %2, i32* %result
  br label %whileFirst

whileEnd:                                         ; preds = %whileFirst
  %result4 = load i32, i32* %result
  ret i32 %result4
}
