#! /bin/sh

# UCLA CS 111 Lab 1c - Test that valid syntax is processed correctly.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
echo hi && (wc && wc -l) < command-internals.h

echo test || fail

EOF

cat >test.exp <<'EOF'
# 1
  hi
  	42 114 828
  0
# 2
  test
EOF

../timetrash -p test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
