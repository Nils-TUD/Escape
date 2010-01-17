if ('a' == 'a') then
	$a = 4;
	echo "test1" > f1.txt;
	echo "test2" > f2.txt;
	echo "test3" > f3.txt;
	echo "test4" > f4.txt;
	echo "test5" > f5.txt;
	cat < "file.txt";
	cat f1.txt | wc &;
	cat f2.txt | wc &;
	cat f3.txt | wc &;
	cat f4.txt | wc &;
	cat f5.txt | wc &;
	cat f4.txt | wc &;
	cat f5.txt | wc &;
	echo {$a + 2} hier 1>&2 > "test.txt";
	echo hier bin ich; echo ich nicht :P;
fi

if (1 < 3) then
	if ($a == 4) then
		$bla = "test";
	else
		$muh = "bla";
	fi
	$b = `echo test | wc`;
	#$b = `echo test`;
	$c = 123 + (4 * 6) ^ 2 - 12;
else
	$c = 'test';
fi
