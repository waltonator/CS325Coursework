; ModuleID = 'mini-c'
source_filename = "mini-c"

define i32 @multiplyNumbers(i32 %n) {
funcBlock:
  %n1 = alloca i32
  store i32 %n, i32* %n1
  %result = alloca i32
  store i32 0, i32* %result
  store i32 0, i32* %result
  %n2 = load i32, i32* %n1
  %0 = icmp sge i32 %n2, 1
  %ifcond = icmp ne i1 %0, false
  br i1 %ifcond, label %if, label %else

if:                                               ; preds = %funcBlock
  %n3 = load i32, i32* %n1
  %n4 = load i32, i32* %n1
  %1 = sub i32 %n4, 1
  %2 = call i32 @multiplyNumbers(i32 %1)
  %3 = mul i32 %n3, %2
  store i32 %3, i32* %result
  br label %cont

else:                                             ; preds = %funcBlock
  store i32 1, i32* %result
  br label %cont

cont:                                             ; preds = %else, %if
  %result5 = load i32, i32* %result
  ret i32 %result5
}

define i32 @rfact(i32 %n) {
funcBlock:
  %n1 = alloca i32
  store i32 %n, i32* %n1
  %n2 = load i32, i32* %n1
  %0 = call i32 @multiplyNumbers(i32 %n2)
  ret i32 %0
}
