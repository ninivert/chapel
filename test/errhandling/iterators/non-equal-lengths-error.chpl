proc willthrow(): string throws {
  throw new owned Error();
  return 777; // note that this line is ignored
}

var RRR = 0..3;

proc test(expr) {
  for zip(RRR, expr) {
    if numLocales < 0 then writeln("hi"); // no-op
  }
}

test( for RRR do willthrow() );
