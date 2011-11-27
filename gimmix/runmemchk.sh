#!/bin/bash
ROMDIR=build/tests
RESULTDIR=build/memchk
TESTS=tests/*.test
GIMMIX=./build/gimmix
DIRS="user kernel diff cli"
TEST=""
VALGRINDOPTS="-q --leak-check=full --show-reachable=yes"

for i in "$@"; do
	case "$i" in
		--help)
			echo "Usage: $0 [<test>...]"
			echo "	<test> is expected as \"tests/<test>.mms\""
			exit 0
			;;
		*)
			if [ "$TEST" != "" ]; then
				TEST="$TEST $i"
			else
				TEST="$i"
			fi
			;;
	esac
done

# first, ensure that everything is up to date
cd org && make && cd .. && make
if [ $? -ne 0 ]; then
	exit 1
fi

# remove all gcda-files
find build/src -type f -name "*.gcda" -print0 | xargs -0 rm -f

mkdir $RESULTDIR 2>/dev/null || true

TESTS=0
SUCC=0
FAILED=0

runTest() {
	NAME=`basename $1 .test`
	CMDS=`sed -n 1,1p $1`
	echo -n "Running $2-test $NAME..."
	ROM="$ROMDIR/$2/$NAME"

	if [ "$2" = "cli" ]; then
		if [ -f tests/$2/$NAME.mms ]; then
			valgrind --log-file=$RESULTDIR/$2/$NAME.log $VALGRINDOPTS \
				$GIMMIX --test -i --script=tests/$2/$NAME.script -l $ROM.mmx > /dev/null
		else
			valgrind --log-file=$RESULTDIR/$2/$NAME.log $VALGRINDOPTS \
				$GIMMIX --test -i --script=tests/$2/$NAME.script > /dev/null
		fi
	elif [ "$2" = "kernel" ] || [ "$2" = "diff" ]; then
		valgrind --log-file=$RESULTDIR/$2/$NAME.log $VALGRINDOPTS \
			$GIMMIX --test -l $ROM.mmx
	else
		valgrind --log-file=$RESULTDIR/$2/$NAME.log $VALGRINDOPTS \
			$GIMMIX --test --user -l $ROM.mmx
	fi
	
	if [ -s "$RESULTDIR/$2/$NAME.log" ]; then
		echo -e '\033[0;31mFAILED\033[0m'
		let FAILED++
	else
		echo -e '\033[0;32mSUCCESS\033[0m'
		let SUCC++
	fi
	let TESTS++
}

if [ "$TEST" != "" ]; then
	for t in $TEST; do
		runTest "tests/$t.test" `dirname $t`
	done
else
	for dir in $DIRS; do
		mkdir -p $RESULTDIR/$dir || true
	
		for i in tests/$dir/*.test; do
			runTest $i $dir
		done
	done
fi

echo -e -n "\033[1mSummary:\033[0m $TESTS tests, `expr $SUCC + $FAILED` comparisons,"
echo " $SUCC succeeded, $FAILED failed"

