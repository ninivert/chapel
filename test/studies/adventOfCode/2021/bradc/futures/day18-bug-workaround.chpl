use IO;

class Node {
  var val: int;
  var left, right: owned Node?;

  proc init(val: int) {
    this.val = val;
  }

  proc init(ref l: Node?, ref r: Node?) {
    this.val = -1;
    this.left = l;
    this.right = r;
  }
}

var line: string = "[1,1]";
var pos = 0;
writeln(lineToTree(line, pos));


proc lineToTree(line: string, ref pos: int): owned Node {
  var ch = line[pos];
  if line[pos] == "[" {
    var left, right: owned Node?;
    pos += 1;  // skip past "["
    left = lineToTree(line, pos);
    pos += 1;  // skip past ","
    right = lineToTree(line, pos);
    pos += 1;
    return new Node(left, right);
  } else if ch >= "0" && ch <= "9" {
    pos += 1;
    return new Node(ch:int);
  } else {
    halt("Illegal input: ", ch);
  }
}

