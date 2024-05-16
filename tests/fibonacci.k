def binary : 1 (x, y) y;

decl printd(x);


def fib(x) 
    if x < 2 then
        0
    else if (x < 3) then 
        1 
    else 
        fib(x-1) + fib(x-2);

def fibiterative(x) 
     spawn a = 1, b = 1, c endspawn
  (for i = 4, i < x in
     c = a + b :
     a = b :
     b = c) :
  b;

//printd(fib(8));
printd(fibiterative(8));