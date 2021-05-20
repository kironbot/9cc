#!/bin/bash
cat <<EOF | cc -xc -c -o tmp2.o -
int ret3() {return 3;}
int ret8() {return 8;}
int add(int x, int y) {return x+y;}
int sub(int x, int y) {return x-y;}
int add6(int a, int b, int c, int d, int e, int f) {
    return a+b+c+d+e+f;
}
EOF


assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 1  "int main() { return 1;}"
assert 42 "int main() { return 42;}"
assert 21 'int main () { return 5+20-4; }'
assert 41 'int main () { return  12 + 34 - 5 ; }'
assert 6  'int main () { return 2 * 3; }'
assert 4  'int main () { return (3+5)/2; }'
assert 10 'int main () { return + 10; }'
assert 1  'int main () { return -(1+2) + 4; }'
assert 1  'int main () { return 3==3; }'
assert 1  'int main () { return 1 != 0; }'
assert 3  'int main () { int a; a=3; return a; }'
assert 42 'int main () { int a; int b; a=40; b = 2; return a + b; }'
assert 4  'int main () { int foo; foo=4; return foo; }'
assert 2  'int main () { int x1; x1 = 2; return x1; }'
assert 7  'int main () { int foo; int bar; foo=4; bar = 3; return foo + bar; }'
assert 3  'int main () { if (0) return 2; return 3; }'
assert 3  'int main () { if (1-1) return 1; return 3; }' 
assert 2  'int main () { if (1) return 2; return 3; }' 
assert 2  'int main () { if (0) return 1; else return 2; }'
assert 5  'int main () { int i=0; while(i<5) i=i+1; return i; }'
assert 3  'int main () { for (;;) return 3; return 9; }'
assert 55 'int main () { int i; int s; i=0; s=0; for (i=0; i<=10; i=i+1) s=s+i; return s; }'
assert 3  'int main () { {1; 2;} return 3; }'
assert 55 'int main () {int i; int j; i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'
assert 3  'int main () { return ret3(); }'
assert 8  'int main () { return ret8(); }'
assert 8  'int main () { return add(2, 6); }'
assert 2  'int main () { return sub(5, 3); }'
assert 21 'int main () { return add6(1,2,3,4,5,6); }'
assert 7  'int main() { return f(3, 4); } int f(int x, int y) {return x + y; }'
assert 55 'int main() { return fib(9); } int fib(int x) {if (x <= 1) return 1; return fib(x-1) + fib(x-2); }'
assert 3  'int main() { int x=3; return *&x; }'
assert 3  'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
assert 5  'int main() { int x=3; int y=5; return *(&x+1); }'
assert 3  'int main() { int x=3; int y=5; return *(&y-1); }'
assert 8  'int main() { int x=3; int y=5; return foo(&x, y); } int foo(int *x, int y) {return *x + y; }'
assert 3  'int main() { int x[2]; *x = 3; return *x;}'
assert 3  'int main() { int x[2]; int *y = &x; *y = 3; return *x;}'

echo OK