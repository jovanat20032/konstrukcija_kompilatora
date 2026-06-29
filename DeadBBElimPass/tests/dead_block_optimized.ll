; ModuleID = '/home/minaproticc/konstrukcija_kompilatora/DeadBBElimPass/tests/dead_block.ll'
source_filename = "/home/minaproticc/konstrukcija_kompilatora/DeadBBElimPass/tests/dead_block.ll"

define i32 @test_dead_block(i32 %x) {
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %positive, label %negative

positive:                                         
  ret i32 1

negative:                                         
}
