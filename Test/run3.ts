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
