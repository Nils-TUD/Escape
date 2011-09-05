#!/bin/bash
ROMDIR=build/tests
RESULTDIR=build/coverage
TESTS=tests/*.test
GIMMIX=./build/gimmix
ROOT=$(dirname $(readlink -f $0))
DIRS="user kernel diff cli"

if [ $# -ne 1 ]; then
	echo "Usage: $0 all|total|<testname>" 1>&2
	echo "	\"all\" will generate individual coverage-results and the total coverage" 1>&2
	echo "	\"total\" will just generate the total coverage" 1>&2
	echo "	<testname> is expected to be like \"tests/<testname>.test\", so e.g. \"./runcov.sh user/fadd\"" 1>&2
	exit 1
fi

# first, ensure that everything is up to date
cd org && make && cd .. && make
if [ $? -ne 0 ]; then
	exit 1
fi

runTest() {
	echo -n "Running $1..."
	ROM="$ROMDIR/$1"
	if [[ "$1" =~ ^cli/ ]]; then
		if [ -f tests/$1.mms ]; then
			$GIMMIX --test -i --script=tests/$1.script -l $ROMDIR/$1.mmx > /dev/null
		else
			$GIMMIX --test -i --script=tests/$1.script > /dev/null
		fi
	elif [[ "$1" =~ ^(kernel|diff)/ ]]; then
		$GIMMIX --test -l $ROM.mmx
	else
		$GIMMIX --test --user -l $ROM.mmx
	fi
	echo "DONE"
}

generateCoverage() {
	mkdir -p $1 || true
	# generate the coverage for all c-files
	for file in `find src -name *.c`; do
		# generate output with gcov
		COVDIR=`dirname $file`
		(cd src && gcov -p -o $ROOT/build/$COVDIR $file 2>/dev/null 1>/dev/null)
		mkdir -p $1/$COVDIR
		# move gcov-file to coverage-dir
		COVFILE=${file//src\//}
		mv src/${COVFILE//\//#}.gcov $1/$file.gcov
		
		# generate stats for this file and the result-file
		./tools/gencovfilestats.rb $1/$file.gcov > $1/$file.gcov.stats
		./tools/gencovfile.rb $file.gcov $ROOT/$1 > $1/$file.gcov.html
	done
	
	# remove a few gcov-files for non-c-files
	find src -type f -name "*.gcov" -print0 | xargs -0 rm -f
	
	# finally generate the index for this test
	./tools/gencovtestindex.rb $1/src > $1/index.html
}

mkdir $RESULTDIR 2>/dev/null || true

(cd tests && ../tools/gencovindex.rb {user,kernel,diff}/*.test > ../$RESULTDIR/index.html)

if [ "$1" != "all" ] && [ "$1" != "total" ]; then
	if [ ! -f tests/$1.test ]; then
		echo "File tests/$1.test does not exist" 2>&1
		exit 1
	fi
	
	# remove previous results
	find build/src -type f -name "*.gcda" -print0 | xargs -0 rm -f
	
	# run the program
	runTest $1

	generateCoverage $RESULTDIR/$1
fi

# generate coverage of each test, individually
if [ "$1" = "all" ]; then
	for dir in $DIRS; do
		for i in tests/$dir/*.test; do
			NAME=`basename $i .test`
		
			# remove previous results
			find build/src -name *.gcda | xargs rm
	
			# run the program
			runTest $dir/$NAME
		
			generateCoverage $RESULTDIR/$dir/$NAME
		done
	done
fi

if [ "$1" = "all" ] || [ "$1" = "total" ]; then
	# remove all gcda-files
	find build/src -type f -name "*.gcda" -print0 | xargs -0 rm -f

	# generate a total-coverage
	for dir in $DIRS; do
		for i in tests/$dir/*.test; do
			NAME=`basename $i .test`

			# run the program
			runTest $dir/$NAME
		done
	done
	# now generate coverage for all runs
	echo -n "Generating coverage for all tests..."
	generateCoverage $RESULTDIR/total/total
	echo "DONE"
fi

# better remove all gcda-files now
find build/src -type f -name "*.gcda" -print0 | xargs -0 rm -f

