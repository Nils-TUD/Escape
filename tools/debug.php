<?php
if($argc < 2) {
	printf("Usage: %s <file> [--user]\n",$argv[0]);
	return 1;
}

define('TYPE',1);
if($argv > 2 && $argv[2] == '--user') {
	$kernel = false;
	define('ADDRESS',2);
	define('SIZE',3);
	define('CALLERNAME',4);
}
else {
	$kernel = true;
	define('PROC',2);
	define('ADDRESS',3);
	define('SIZE',4);
	define('CALLERNAME',5);
}

$allocs = array();
$matches = array();
$content = implode('',file($argv[1]));
if($kernel)
	preg_match_all('/\[(A|F)\] p=([^ ]+) a=([\da-f]+) s=(\d+) c=([a-z0-9A-Z_]+)/',$content,$matches);
else
	preg_match_all('/\[(A|F)\] a=([\da-f]+) s=(\d+) c=([A-Fa-f0-9]+), ([A-Fa-f0-9]+)/',$content,$matches);

foreach($matches[0] as $k => $v) {
	$addr = $matches[ADDRESS][$k];
	if(!isset($allocs[$addr]))
		$allocs[$addr] = array(0,0,0,'','');
	if($matches[TYPE][$k] == 'A') {
		$allocs[$addr][0]++;
		$allocs[$addr][1] = $matches[SIZE][$k];
		if($kernel)
			$allocs[$addr][2] = $matches[PROC][$k];
		else
			$allocs[$addr][2] = '';
		$allocs[$addr][3] = $matches[CALLERNAME][$k];
		if(!$kernel)
			$allocs[$addr][4] = $matches[CALLERNAME + 1][$k];
	}
	else
		$allocs[$addr][0]--;
}

foreach($allocs as $addr => $open) {
	if($open[0] > 0)
		echo $addr.' => '.$open[0].' (size='.$open[1].', proc='.$open[2].', caller='.strtolower($open[3]).', '.strtolower($open[4]).')'."\n";
}
?>
