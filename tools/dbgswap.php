<?php
if($argc < 2) {
	printf("Usage: %s <file>\n",$argv[0]);
	return 1;
}

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
	$block = $matches[8][$k];
	if($type == 'out')
		$swapped[$reg.':'.$index] = $block;
	else if(!isset($swapped[$reg.':'.$index]) || $swapped[$reg.':'.$index] != $block) {
		echo "Error: Swapped in from block ".$block." for ".$reg.':'.$index.", but expected ";
		echo isset($swapped[$reg.':'.$index]) ? $swapped[$reg.':'.$index] : '??';
		echo "\n";
	}
}
?>
