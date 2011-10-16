<?php
if($argc < 2) {
	printf("Usage: %s <file> [<n>]\n",$argv[0]);
	return 1;
}

$n = $argc == 3 ? $argv[2] : 20;
$swapped = array();
$matches = array();
$content = file_get_contents($argv[1]);
preg_match_all(
	'/(IN|OUT): (\d+) of region ([0-9a-f]+) \(block (\d+)\)/',
	$content,
	$matches
);

foreach($matches[0] as $k => $v) {
	$type = $matches[1][$k];
	$index = $matches[2][$k];
	$reg = $matches[3][$k];
	$block = $matches[4][$k];
	if(!isset($swapped[$reg.':'.$index]))
		$swapped[$reg.':'.$index] = array(0,0);
	if($type == 'OUT')
		$swapped[$reg.':'.$index][0]++;
	else
		$swapped[$reg.':'.$index][1]++;
}

function compare($a,$b) {
	return ($b[0] + $b[1]) - ($a[0] + $a[1]);
}
uasort($swapped,"compare");

echo "Top $n:\n";
$i = 0;
$in = 0;
$out = 0;
foreach($swapped as $k => $v) {
	if($i++ < $n)
		echo $k.': '.$v[0].' '.$v[1]."\n";
	$out += $v[0];
	$in += $v[1];
}
echo "Outs: ".$out."\n";
echo "Ins: ".$in."\n";
?>
