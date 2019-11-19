; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @addition(i32 %n, i32 %m) {
funcBlock:
  %m2 = alloca i32
  %n1 = alloca i32
  store i32 %n, i32* %n1
  store i32 %m, i32* %m2
  %result = alloca i32
  store i32 0, i32* %result
  %n3 = load i32, i32* %n1
  %m4 = load i32, i32* %m2
  %0 = add i32 %n3, %n3
  store i32 %0, i32* %result
  %n5 = load i32, i32* %n1
  %1 = icmp eq i32 %n5, %n5
  %ifcond = icmp ne i1 %1, false
  br i1 %ifcond, label %if, label %else

if:                                               ; preds = %funcBlock
  %n6 = load i32, i32* %n1
  %m7 = load i32, i32* %m2
  %2 = add i32 %n6, %n6
  %3 = call i32 @print_int(i32 %2)
  br label %cont

else:                                             ; preds = %funcBlock
  %n8 = load i32, i32* %n1
  %m9 = load i32, i32* %m2
  %4 = mul i32 %n8, %n8
  %5 = call i32 @print_int(i32 %4)
  br label %cont

cont:                                             ; preds = %else, %if
  %result10 = load i32, i32* %result
  ret i32 %result10
}
