#
# test while statements
#
var i=1

while i<=10 {
  var z=(i+1)/2
  print i, " : ", i*i, " usrfunc = ", dsqr(i, z)
  i=i+1
}

#
# test procedures
#
var global=99

proc printglobal {
    print "global= ", global
}
proc myfunc {
    var global=123
    printglobal
}

printglobal
myfunc
printglobal

