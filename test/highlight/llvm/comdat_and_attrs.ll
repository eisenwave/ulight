$foo = comdat any

define dso_local i32 @main() #0 comdat($foo) {
entry:
  ret i32 0
}

attributes #0 = { nounwind uwtable }
