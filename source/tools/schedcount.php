<?php
if($argc != 2)
	exit("Usage: " . $argv[0] . " <file>\n");

$lines = file($argv[1]);
$lastc = 0;
$lasti = -2;
for($i = 0; $i < count($lines); $i++) {
	if($lasti == $i - 1) {
		if(!preg_match('/^A\d+: (\d+)$/',$lines[$i],$matches) || $matches[1] != $lastc + 1)
			echo "Error on line " . $i . "\n";
	}
	if(preg_match('/^B\d+: (\d+)$/',$lines[$i],$matches)) {
		$lastc = $matches[1];
		$lasti = $i;
	}
}
?>
