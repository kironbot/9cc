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

assert 1 "return 1;"
assert 42 "return 42;"
assert 21 'return 5+20-4;'
assert 41 'return  12 + 34 - 5 ;'
assert 6 'return 2 * 3;'
assert 4 'return (3+5)/2;'
assert 10 'return + 10;'
assert 1 'return -(1+2) + 4;'
assert 1 'return 3==3;'
assert 1 'return 1 != 0;'

assert 3 'a=3; return a;'
assert 42 'a=40; b = 2; return a + b;'

assert 4 'foo=4; return foo;'
assert 2 'x1 = 2; return x1;'
assert 7 'foo=4; bar = 3; return foo + bar;'

assert 3 'if (0) return 2; return 3;'
assert 3 'if (1-1) return 1; return 3;' 
assert 2 'if (1) return 2; return 3;' 
assert 2 'if (0) return 1; else return 2;'

assert 5 'i=0; while(i<5) i=i+1; return i;'

assert 3 'for (;;) return 3; return 9;'
assert 55 'i=0; s=0; for (i=0; i<=10; i=i+1) s=s+i; return s;'

assert 3 '{1; 2;} return 3;'
assert 55 'i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;'

assert 3 'return ret3();'
assert 8 'return ret8();'

assert 8 'return add(2, 6);'
assert 2 'return sub(5, 3);'
assert 21 'return add6(1,2,3,4,5,6);'

echo OK