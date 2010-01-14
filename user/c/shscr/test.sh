if ('a' == 'a') then
	$a = 4
	echo "muh" und mehr
	echo "test" 2>&1
	echo {$a + 2} hier 1>&2 > "test.txt"
	ls >>hier.txt
	cat <datei.txt > log.txt | wc -l | wc
fi

if (1 < 3) then
	if ($a == 4) then
		$asd = "hier"
	else
		$muh = "bla"
	fi
	$b = `echo {$a + $muh}`
	$c = 123 + (4 * 6) ^ 2 - 12
else
	$c = 'test'
fi