var x = 1;

proc foo(n: int, ref A: [1..n] int) {
  for i in 1..n {
    A(i) = x;
    x += 1;
  }
}

var A: [1..12] int;
writeln(A);
foo(12, A[1..12].reindex(1..12));
writeln(A);
foo(6, A[1..12 by 2].reindex(1..6));
writeln(A);
foo(4, A[1..12 by 3].reindex(1..4));
writeln(A);
foo(3, A[1..12 by 4].reindex(1..3));
writeln(A);
foo(2, A[1..12 by 6].reindex(1..2));
writeln(A);
