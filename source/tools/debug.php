<?php
if($argc < 2) {
	printf("Usage: %s <file>\n",$argv[0]);
	return 1;
}

define('TYPE',1);
define('ADDRESS',2);
define('SIZE',3);
define('CALLERNAME',4);

$allocs = array();
$matches = array();
$content = implode('',file($argv[1]));
preg_match_all('/\[(A|F)\] ([\da-f]+) (\d+) (.*)/',$content,$matches);

foreach($matches[0] as $k => $v) {
	$addr = $matches[ADDRESS][$k];
	if(!isset($allocs[$addr]))
		$allocs[$addr] = array(0,0,'');
	if($matches[TYPE][$k] == 'A') {
		if($allocs[$addr][0] > 0)
			echo "ALREADY ALLOCATED!\n";
		$allocs[$addr][0]++;
		$allocs[$addr][1] = $matches[SIZE][$k];
		$allocs[$addr][2] = $matches[CALLERNAME][$k];
	}
	else
		$allocs[$addr][0]--;
}

foreach($allocs as $addr => $open) {
	if($open[0] != 0)
		echo $addr.' => '.$open[0].' (size='.$open[1].', caller='.$open[2].')'."\n";
}
?>
