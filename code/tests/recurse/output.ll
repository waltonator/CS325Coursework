; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @addNumbers(i32 %n) {
funcBlock:
  %n1 = alloca i32
  store i32 %n, i32* %n1
  %result = alloca i32
  store i32 0, i32* %result
  store i32 0, i32* %result
  %n2 = load i32, i32* %n1
  %0 = icmp ne i32 %n2, 0
  %ifcond = icmp ne i1 %0, false
  br i1 %ifcond, label %if, label %else

if:                                               ; preds = %funcBlock
  %n3 = load i32, i32* %n1
  %n4 = load i32, i32* %n1
  %1 = sub i32 %n4, 1
  %2 = call i32 @addNumbers(i32 %1)
  %3 = add i32 %n3, %2
  store i32 %3, i32* %result
  br label %cont

else:                                             ; preds = %funcBlock
  %n5 = load i32, i32* %n1
  store i32 %n5, i32* %result
  br label %cont

cont:                                             ; preds = %else, %if
  %result6 = load i32, i32* %result
  %4 = call i32 @print_int(i32 %result6)
  %result7 = load i32, i32* %result
  ret i32 %result7
}

define i32 @recursion_driver(i32 %num) {
funcBlock:
  %num1 = alloca i32
  store i32 %num, i32* %num1
  %num2 = load i32, i32* %num1
  %0 = call i32 @addNumbers(i32 %num2)
  ret i32 %0
}
