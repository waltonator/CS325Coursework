; ModuleID = 'mini-c'
source_filename = "mini-c"

define i32 @factorial(i32 %n) {
funcBlock:
  %n1 = alloca i32
  store i32 %n, i32* %n1
  %i = alloca i32
  store i32 0, i32* %i
  %factorial = alloca i32
  store i32 0, i32* %factorial
  store i32 1, i32* %factorial
  store i32 1, i32* %i
  br label %whileFirst

whileFirst:                                       ; preds = %whileLoop, %funcBlock
  %i2 = load i32, i32* %i
  %n3 = load i32, i32* %n1
  %0 = icmp sle i32 %i2, %n3
  %whilecond = icmp ne i1 %0, false
  br i1 %whilecond, label %whileLoop, label %whileEnd

whileLoop:                                        ; preds = %whileFirst
  %factorial4 = load i32, i32* %factorial
  %i5 = load i32, i32* %i
  %1 = mul i32 %factorial4, %i5
  store i32 %1, i32* %factorial
  %i6 = load i32, i32* %i
  %2 = add i32 %i6, 1
  store i32 %2, i32* %i
  br label %whileFirst

whileEnd:                                         ; preds = %whileFirst
  %factorial7 = load i32, i32* %factorial
  ret i32 %factorial7
}
