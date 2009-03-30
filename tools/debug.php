<?php
if($argc < 2) {
	printf("Usage: %s <file>\n",$argv[0]);
	return 1;
}

$allocs = array();
$matches = array();
$content = implode('',file($argv[1]));
preg_match_all('/(Alloc|Free ) of (\d+) -> (0x[\da-f]+)/',$content,$matches);

foreach($matches[0] as $k => $v) {
	$addr = $matches[3][$k];
	$size = $matches[2][$k];
	if(!isset($allocs[$addr]))
		$allocs[$addr] = array(0,0);
	if($matches[1][$k] == 'Alloc') {
		$allocs[$addr][0]++;
		$allocs[$addr][1] = $size;
	}
	else
		$allocs[$addr][0]--;
}

foreach($allocs as $addr => $open) {
	if($open[0] > 0)
		echo $addr.' => '.$open[0].' ('.$open[1].')'."\n";
}
?>
