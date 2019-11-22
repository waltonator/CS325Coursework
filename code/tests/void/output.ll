; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define void @Void() {
funcBlock:
  %result = alloca i32
  store i32 0, i32* %result
  store i32 0, i32* %result
  %result1 = load i32, i32* %result
  %0 = call i32 @print_int(i32 %result1)
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
  %result4 = load i32, i32* %result
  %3 = call i32 @print_int(i32 %result4)
  br label %whileFirst

whileEnd:                                         ; preds = %whileFirst
  ret void
}
