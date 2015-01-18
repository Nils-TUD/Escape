#!/usr/bin/env php
<?php
if($argc != 2)
	exit("Usage: {$argv[0]} <file>\n");

function compare($a,$b) {
	$as = substr_count($a,'/');
	$bs = substr_count($b,'/');
	if($as > $bs)
		return -1;
	if($bs > $as)
		return 1;
	return strcasecmp($a,$b);
}

$first = -1;
$last = -1;
$lines = file($argv[1]);
for($i = 0; $i < count($lines); ++$i) {
	$line = $lines[$i];
	if(substr($line,0,8) == '#include') {
		if($first == -1)
			$first = $i;
	}
	else if($first != -1 && trim($line) != '') {
		$last = $i - 1;
		break;
	}
}

if($first != -1) {
	if($last == -1)
		$last = $i - 1;

	$f = fopen($argv[1],"w");

	for($i = 0; $i < $first; ++$i)
		fwrite($f,$lines[$i]);

	$gincs = array();
	$lincs = array();
	for($i = $first; $i <= $last; ++$i) {
		if(trim($lines[$i]) != '') {
			if(preg_match('/^#include\s+"/',$lines[$i]))
				$lincs[] = $lines[$i];
			else
				$gincs[] = $lines[$i];
		}
	}

	usort($gincs,"compare");
	usort($lincs,"compare");

	foreach($gincs as $inc)
		fwrite($f,$inc);
	if(count($lincs) > 0) {
		if(count($gincs) > 0)
			fwrite($f,"\n");
		foreach($lincs as $inc)
			fwrite($f,$inc);
	}
	if(count($lincs) > 0 || count($gincs) > 0)
		fwrite($f,"\n");

	for($i = $last + 1; $i < count($lines); ++$i)
		fwrite($f,$lines[$i]);
	fclose($f);
}
?>