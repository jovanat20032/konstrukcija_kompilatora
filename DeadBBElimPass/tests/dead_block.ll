define i32 @test_dead_block(i32 %x) {
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %positive, label %negative

positive:
  ret i32 1

negative:
  ret i32 0

dead:
  ret i32 100
}