func f(x,y) {
  print "x=", x, " y=", y
  return x+y
}

print f(1,2)
print f(f(1,4),3)
print f(7,f(8,9))
