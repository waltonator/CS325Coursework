; ModuleID = 'mini-c'
source_filename = "mini-c"

define i1 @palindrome(i32 %number) {
funcBlock:
  %number1 = alloca i32
  store i32 %number, i32* %number1
  %t = alloca i32
  store i32 0, i32* %t
  %rev = alloca i32
  store i32 0, i32* %rev
  %rmndr = alloca i32
  store i32 0, i32* %rmndr
  %result = alloca i1
  store i1 false, i1* %result
  store i32 0, i32* %rev
  store i1 false, i1* %result
  %number2 = load i32, i32* %number1
  store i32 %number2, i32* %t
  br label %whileFirst

whileFirst:                                       ; preds = %whileLoop, %funcBlock
  %number3 = load i32, i32* %number1
  %0 = icmp sgt i32 %number3, 0
  %whilecond = icmp ne i1 %0, false
  br i1 %whilecond, label %whileLoop, label %whileEnd

whileLoop:                                        ; preds = %whileFirst
  %number4 = load i32, i32* %number1
  %1 = srem i32 %number4, 10
  store i32 %1, i32* %rmndr
  %rev5 = load i32, i32* %rev
  %2 = mul i32 %rev5, 10
  %rmndr6 = load i32, i32* %rmndr
  %3 = add i32 %2, %rmndr6
  store i32 %3, i32* %rev
  %number7 = load i32, i32* %number1
  %4 = sdiv i32 %number7, 10
  store i32 %4, i32* %number1
  br label %whileFirst

whileEnd:                                         ; preds = %whileFirst
  %t8 = load i32, i32* %t
  %rev9 = load i32, i32* %rev
  %5 = icmp eq i32 %t8, %rev9
  %ifcond = icmp ne i1 %5, false
  br i1 %ifcond, label %if, label %else

if:                                               ; preds = %whileEnd
  store i1 true, i1* %result
  br label %cont

else:                                             ; preds = %whileEnd
  store i1 false, i1* %result
  br label %cont

cont:                                             ; preds = %else, %if
  %result10 = load i1, i1* %result
  ret i1 %result10
}
