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

assert 1 "main() { return 1;}"
assert 42 "main() { return 42;}"
assert 21 'main () { return 5+20-4; }'
assert 41 'main () { return  12 + 34 - 5 ; }'
assert 6 'main () { return 2 * 3; }'
assert 4 'main () { return (3+5)/2; }'
assert 10 'main () { return + 10; }'
assert 1 'main () { return -(1+2) + 4; }'
assert 1 'main () { return 3==3; }'
assert 1 'main () { return 1 != 0; }'

assert 3 'main () { a=3; return a; }'
assert 42 'main () { a=40; b = 2; return a + b; }'

assert 4 'main () { foo=4; return foo; }'
assert 2 'main () { x1 = 2; return x1; }'
assert 7 'main () { foo=4; bar = 3; return foo + bar; }'

assert 3 'main () { if (0) return 2; return 3; }'
assert 3 'main () { if (1-1) return 1; return 3; }' 
assert 2 'main () { if (1) return 2; return 3; }' 
assert 2 'main () { if (0) return 1; else return 2; }'

assert 5 'main () { i=0; while(i<5) i=i+1; return i; }'

assert 3 'main () { for (;;) return 3; return 9; }'
assert 55 'main () { i=0; s=0; for (i=0; i<=10; i=i+1) s=s+i; return s; }'

assert 3 'main () { {1; 2;} return 3; }'
assert 55 'main () { i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

assert 3 'main () { return ret3(); }'
assert 8 'main () { return ret8(); }'

assert 8 'main () { return add(2, 6); }'
assert 2 'main () { return sub(5, 3); }'
assert 21 'main () { return add6(1,2,3,4,5,6); }'

assert 7 'main() { return f(3, 4); } f(x, y) {return x + y; }'
assert 55 'main() { return fib(9); } fib(x) {if (x <= 1) return 1; return fib(x-1) + fib(x-2); }'

assert 3 'main() { x=3; return *&x; }'
assert 3 'main() { x=3; y=&x; z=&y; return **z; }'
assert 5 'main() { x=3; y=5; return *(&x+8); }'

echo OK