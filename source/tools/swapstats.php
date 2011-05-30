<?php
if($argc < 2) {
	printf("Usage: %s <file> [<n>]\n",$argv[0]);
	return 1;
}

$n = $argc == 3 ? $argv[2] : 20;
$swapped = array();
$matches = array();
$content = implode('',file($argv[1]));
preg_match_all(
	'/Swap(in|out) ([0-9a-f]+):(\d+) \(first=([a-f0-9:]+) (.*?):(\d+)\) (from|to) blk (\d+)/',
	$content,
	$matches
);

foreach($matches[0] as $k => $v) {
	$type = $matches[1][$k];
	$reg = $matches[2][$k];
	$index = $matches[3][$k];
	$virt = $matches[4][$k];
	$pname = $matches[5][$k];
	$pid = $matches[6][$k];
	if($type == 'out') {
		if(!isset($swapped[$reg.':'.$index]))
			$swapped[$reg.':'.$index] = array(1,$virt,$pname,$pid);
		else {
			$c = $swapped[$reg.':'.$index][0];
			$swapped[$reg.':'.$index] = array($c + 1,$virt,$pname,$pid);
		}
	}
}

function compare($a,$b) {
	return $b[0] - $a[0];
}
usort($swapped,"compare");

echo "Top $n:\n";
$i = 0;
$total = 0;
foreach($swapped as $k => $v) {
	if($i++ < $n)
		echo $v[0].' '.$v[1].':'.$v[2].':'.$v[3]."\n";
	$total += $v[0];
}
echo "Avg: ".($total / count($swapped))."\n";
?>
