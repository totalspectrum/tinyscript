func f(x,y) {
  print "x=", x, " y=", y
  return x+y
}

print f(1,2)
print f(f(1,4),3)
print f(7,f(8,9))

func g(x) {
  return x<<1
  return x
}

print g(2)
print g(3)

func in_if(x) {
  if x < 10 {
    return x
  }
  return 42
}

func inner(x) {
  return 555
}

func outer(x) {
  var y = inner(x)
  return x + y
}

print in_if(8)
print in_if(11)
print outer(445)
print outer(in_if(4))
print in_if(outer(1))
