const D = {1..9, 1..9};
var A: [D] real;

forall (i,j) in D with (ref A) do
  A[i,j] = i + j/10.0;

printArray(A[5..6, 7..8]);
printArray2(A[5..6, 7..8]);
printArray3(A[5..6, 7..8]);
printArray4(A[5..6, 7..8]);
printArray5(A[5..6, 7..8]);

proc printArray(X) {
  writeln(X);
  writeln();
}

proc printArray2(X: []) {
  writeln(X);
  writeln();
}

proc printArray3(X: [] real) {
  writeln(X);
  writeln();
}

proc printArray4(X: [5..6, 7..8] real) {
  writeln(X);
  writeln();
}

proc printArray5(X: [?D] ?t) {
  writeln(X);
  writeln(D);
  writeln(t:string);
}
