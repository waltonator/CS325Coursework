; ModuleID = 'mini-c'
source_filename = "mini-c"

@test = global i32 0

declare i32 @print_int(i32)

define i32 @fibonacci(i32 %n) {
funcBlock:
  %n1 = alloca i32
  store i32 %n, i32* %n1
  %first = alloca i32
  store i32 0, i32* %first
  %second = alloca i32
  store i32 0, i32* %second
  %next = alloca i32
  store i32 0, i32* %next
  %c = alloca i32
  store i32 0, i32* %c
  %total = alloca i32
  store i32 0, i32* %total
  %n2 = load i32, i32* %n1
  %0 = call i32 @print_int(i32 %n2)
  store i32 0, i32* %first
  store i32 1, i32* %second
  store i32 1, i32* %c
  store i32 0, i32* %total
  br label %whileFirst

whileFirst:                                       ; preds = %cont, %funcBlock
  %c3 = load i32, i32* %c
  %n4 = load i32, i32* %n1
  %1 = icmp slt i32 %c3, %n4
  %whilecond = icmp ne i1 %1, false
  br i1 %whilecond, label %whileLoop, label %whileEnd

whileLoop:                                        ; preds = %whileFirst
  %c5 = load i32, i32* %c
  %2 = icmp sle i32 %c5, 1
  %ifcond = icmp ne i1 %2, false
  br i1 %ifcond, label %if, label %else

if:                                               ; preds = %whileLoop
  %c6 = load i32, i32* %c
  store i32 %c6, i32* %next
  br label %cont

else:                                             ; preds = %whileLoop
  %first7 = load i32, i32* %first
  %second8 = load i32, i32* %second
  %3 = add i32 %first7, %second8
  store i32 %3, i32* %next
  %second9 = load i32, i32* %second
  store i32 %second9, i32* %first
  %next10 = load i32, i32* %next
  store i32 %next10, i32* %second
  br label %cont

cont:                                             ; preds = %else, %if
  %next11 = load i32, i32* %next
  %4 = call i32 @print_int(i32 %next11)
  %c12 = load i32, i32* %c
  %5 = add i32 %c12, 1
  store i32 %5, i32* %c
  %total13 = load i32, i32* %total
  %next14 = load i32, i32* %next
  %6 = add i32 %total13, %next14
  store i32 %6, i32* %total
  br label %whileFirst

whileEnd:                                         ; preds = %whileFirst
  %total15 = load i32, i32* %total
  %7 = call i32 @print_int(i32 %total15)
  %total16 = load i32, i32* %total
  ret i32 %total16
}
