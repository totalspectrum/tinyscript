#
# simple fibonacci demo
#

# calculate fibo(n)
# parameter in n
# returns result in r
var n=0
var r=0
proc fibo {
if (n<2){
r=n
}else{
var s=n  # save our parameter for recursive call
var a=0
n=n-1;fibo;a=r
n=s-2;fibo;r=a+r
n=s
}
}

# convert cycles to milliseconds
proc calcms {
  ms=(cycles+40000)/80000
}

var i=1
var cycles=0
var ms=0
while i<=8 {
  cycles=getcnt()
  n=i; fibo
  cycles=getcnt()-cycles
  calcms
  print "fibo(",i,") = ",r, " ", ms, " ms (", cycles, " cycles)"
  i=i+1
}
