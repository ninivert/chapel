class Params {
  var numProbSizes: int;
  var N: [1..numProbSizes] int;

  proc init() {
    numProbSizes = 3;
    init this;
    for n in N do n = 10;
  }
}

var ListOfParams = new unmanaged Params();

writeln(ListOfParams);

delete ListOfParams;
