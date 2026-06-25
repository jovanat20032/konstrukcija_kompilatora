; ModuleID = '../llvm/lib/Transforms/konstrukcija_kompilatora/OurCSEPass/tests/test1.ll'
source_filename = "../llvm/lib/Transforms/konstrukcija_kompilatora/OurCSEPass/tests/test1.ll"

define i32 @test(i32 %a, i32 %b) {
entry:
  %x = add i32 %a, %b
  %z = add i32 %x, %x
  ret i32 %z
}
