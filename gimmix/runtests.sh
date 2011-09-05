#!/bin/bash
ROMDIR=build/tests
RESULTDIR=build/testresults
TESTS=tests/*.test
GIMMIX=./build/gimmix
MMIX=./org/build/mmix
MMMIX=./org/build/mmmix
DIRS="user kernel diff cli"
QUIET=0
TEST=""

for i in "$@"; do
	case "$i" in
		--quiet)
			QUIET=1
			;;
		--help)
			echo "Usage: $0 [--quiet] [<test>...]"
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
	
	echo "Running $2-test $NAME..."
	ROM="$ROMDIR/$2/$NAME"
	if [ $2 = "kernel" ]; then
		$GIMMIX --test -l $ROM.mmx --postcmds=$CMDS > $RESULTDIR/$2/$NAME-gimmix.txt
		# mmmix prints some other stuff, therefore we extract the lines starting with $ or m[ or g[
		$MMMIX org/plain.mmconfig $ROM.mmx --postcmds=$CMDS | grep '^\(\$\|m\[\|g\[\)' > $RESULTDIR/$2/$NAME-mmmix.txt
	elif [ $2 = "diff" ]; then
		$GIMMIX --test -l $ROM.mmx --postcmds=$CMDS > $RESULTDIR/$2/$NAME-gimmix.txt
	elif [ $2 = "cli" ]; then
		if [ -f tests/$2/$NAME.mms ]; then
			$GIMMIX --test -i --script=tests/$2/$NAME.script -l $ROMDIR/$2/$NAME.mmx \
				> $RESULTDIR/$2/$NAME-gimmix.txt
		else
			$GIMMIX --test -i --user --script=tests/$2/$NAME.script > $RESULTDIR/$2/$NAME-gimmix.txt
		fi
	else
		$GIMMIX --test -l $ROM.mmx --user --postcmds=$CMDS > $RESULTDIR/$2/$NAME-gimmix.txt
		$MMIX -q $ROM --postcmds=$CMDS > $RESULTDIR/$2/$NAME-mmix.txt
		$MMMIX org/plain.mmconfig $ROM.mmb --postcmds=$CMDS | grep '^\(\$\|m\[\|g\[\)' > $RESULTDIR/$2/$NAME-mmmix.txt
	fi

	# build expected output; use everything beginning in the second line
	sed -n '2,$p' $1 > $RESULTDIR/$2/$NAME-expected.txt
	# append newline
	echo "" >> $RESULTDIR/$2/$NAME-expected.txt

	# if the expected output is empty, don't compare it (sometimes its difficult to write/generate
	# the expected output)
	if [ "`cat $RESULTDIR/$2/$NAME-expected.txt`" != "" ]; then
		echo -en "\tComparing with expected output..."
		if [ "`cmp $RESULTDIR/$2/$NAME-gimmix.txt $RESULTDIR/$2/$NAME-expected.txt 2>&1`" != "" ]; then
			echo -e '\033[0;31mFAILED:\033[0m'
			let FAILED++
			if [ $QUIET -ne 1 ]; then
				diff -C 1 $RESULTDIR/$2/$NAME-gimmix.txt $RESULTDIR/$2/$NAME-expected.txt
			fi
		else
			echo -e '\033[0;32mSUCCESS\033[0m'
			let SUCC++
		fi
	fi

	# don't do the privileged and cli tests with mmix
	if [ $2 = "user" ]; then
		echo -en "\tComparing with mmix output..."
		if [ "`cmp $RESULTDIR/$2/$NAME-gimmix.txt $RESULTDIR/$2/$NAME-mmix.txt 2>&1`" != "" ]; then
			echo -e '\033[0;31mFAILED:\033[0m'
			let FAILED++
			if [ $QUIET -ne 1 ]; then
				diff -C 1 $RESULTDIR/$2/$NAME-gimmix.txt $RESULTDIR/$2/$NAME-mmix.txt
			fi
		else
			echo -e '\033[0;32mSUCCESS\033[0m'
			let SUCC++
		fi
	fi

	# don't do the comparison with mmmix with the tests in diff and cli
	if [ $2 != "diff" ] && [ $2 != "cli" ]; then
		echo -en "\tComparing with mmmix output..."
		if [ "`cmp $RESULTDIR/$2/$NAME-gimmix.txt $RESULTDIR/$2/$NAME-mmmix.txt 2>&1`" != "" ]; then
			echo -e '\033[0;31mFAILED:\033[0m'
			let FAILED++
			if [ $QUIET -ne 1 ]; then
				diff -C 1 $RESULTDIR/$2/$NAME-gimmix.txt $RESULTDIR/$2/$NAME-mmmix.txt
			fi
		else
			echo -e '\033[0;32mSUCCESS\033[0m'
			let SUCC++
		fi
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

