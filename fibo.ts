#
# simple fibonacci demo
#

# calculate fibo(n)

func fibo(n) {
if (n<2){
return n
}else{
return fibo(n-1)+fibo(n-2)
}
}

# convert cycles to milliseconds
func calcms() {
  ms=(cycles+40000)/80000
}

var i=1
var cycles=0
var ms=0
var r=0
while i<=8 {
  cycles=getcnt()
  r=fibo(i)
  cycles=getcnt()-cycles
  calcms()
  print "fibo(",i,") = ",r, " ", ms, " ms (", cycles, " cycles)"
  i=i+1
}
