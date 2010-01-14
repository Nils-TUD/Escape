if ('a' == 'a') then
	$a = 4;
	echo {$a + 2} hier 1>&2 > "test.txt";
	ls -l;
	echo hier bin ich; echo ich nicht :P;
fi

if (1 < 3) then
	if ($a == 4) then
		cat file.txt | wc;
	else
		$muh = "bla";
	fi
	$b = `echo {$a + $muh}`;
	$c = 123 + (4 * 6) ^ 2 - 12;
else
	$c = 'test';
fi
