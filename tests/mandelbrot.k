def unary!(v) if v then 0 else 1;
def unary-(v) 0-v;
def binary : 1 (x, y) 0;
def binary | 5 (LHS, RHS) if LHS then 1 else if RHS then 1 else 0;
def binary > 10 (LHS, RHS) RHS < LHS;
def binary = 9 (LHS RHS) !(LHS < RHS | LHS > RHS);
decl putchard(char);
def printdensity(d) if d > 8 then putchard(32) else if d > 4 then putchard(46) else if d > 2 then putchard(43) else putchard(42);
def mandelconverger(real, imaginary, iterations, constantreal, constantimaginary) 
    if iterations > 255 | (real*real + imaginary*imaginary > 4) 
    then iterations 
    else mandelconverger(real*real - imaginary*imaginary + constantreal, 2*real*imaginary + constantimaginary, iterations + 1, constantreal, constantimaginary);
def mandelconverge(real, imaginary) mandelconverger(real, imaginary, 0 , real, imaginary);
def mandelhelp(xmin, xmax, xstep, ymin, ymax, ystep) for y = ymin, y < ymax, ystep in ((for x = xmin, x < xmax, xstep in printdensity(mandelconverge(x,y))) : putchard(10));
def mandel(realstart, imaginarystart, realmagnification, imagaginarymagnification) mandelhelp(realstart, realstart + realmagnification*78, realmagnification, imaginarystart, imaginarystart + imagaginarymagnification*40, imagaginarymagnification);
mandel(-2.3, -1.3, 0.05, 0.07);