var fd_sequence = 0

# mock file opening function to demonstrate bool coercing
func fopen(name) {
  fd_sequence = fd_sequence + 1
  return fd_sequence
}

var first_fd = fopen(0)
var second_fd = fopen(0)

if first_fd & second_fd {
  print "both are succesfully open, but this never gets printed"
}

if bool(first_fd) & bool(second_fd) {
  print "with bool coercing our logic works!"
}

var lst = list_new(5)
list_push__(lst, 'a', 'b', 'c')
print list_size(lst)
print list_get(lst, 1)

var new_list = list_cat(lst, lst)
print list_size(new_list)
list_free(lst)
print list_pop(new_list)
list_set(new_list, 0, 50)
list_truncate(new_list, 1)
print list_pop(new_list)
print list_size(new_list)
list_free(new_list)

var small_list = list_new(1)
list_push(small_list, 1)
small_list = list_expand(small_list, 2)
list_push(small_list, 1)
print list_size(small_list)
var duped_list = list_dup(small_list)
print list_size(duped_list)

list_free(duped_list)
list_free(small_list)

var format = list_new(10)
list_push__(format, 'x', '%', 'd')
list_push(format, '\n')
printf(format, 42)

