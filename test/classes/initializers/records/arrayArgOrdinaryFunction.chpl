record R {
  var x: int;

  proc init(xVal: int = 0) {
    x = xVal;
    init this;
  }
}

proc arrayArg(arr = [new R(2), new R(3)]) {
  writeln(arr);
}

arrayArg();
