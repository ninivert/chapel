use CTypes;

var x : c_int = 5;
var xp_void : c_ptr(void) = c_ptrTo(x);
var xp = xp_void : c_ptrConst(c_int);
writeln(xp.deref());
var xp_voidAgain : c_ptr(void) = xp : c_ptr(void);
