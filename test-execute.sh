#! /bin/sh

# UCLA CS 111 Lab 1c - Test that valid syntax is processed correctly.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
true || echo test1
false || echo test2 > foo.txt

(echo test5 && echo test6 && echo test8) | tr a-z A-Z > bar.txt

wc < bar.txt && cat bar.txt
EOF

cat >test.exp <<'EOF'
 3  3 18
TEST5
TEST6
TEST8
EOF

../timetrash test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
