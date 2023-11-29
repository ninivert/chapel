use GPU;

config const printResult = false;
config const n = 100;

// testing large data:
// 1. is only meaningful if the reduction is actually done on a device
// 2. times out testing if we use CPU-based reduction, especially if it is a
//    fallback.
// So, override n to be something smaller
var nOverride = n;
extern proc chpl_gpu_can_reduce(): bool;
if !chpl_gpu_can_reduce() then nOverride = 100;

var result: uint(8);
on here.gpus[0] {
  var Arr: [0..#nOverride] uint(8) = 1;

  result = gpuSumReduce(Arr);
}

if printResult then writeln("Result: ", result);

// it is all 1's initially. It'll certainly overflow and the remainder will be
// the expected return value
const expected = nOverride%(max(uint(8))+1);
if result != expected then
  writef("Invalid result. Expected %u, actual %u\n", expected, result);
