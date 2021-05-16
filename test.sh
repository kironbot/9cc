#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
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

echo OK