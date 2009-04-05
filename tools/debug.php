<?php
define('TYPE',1);
define('PID',2);
define('ADDRESS',3);
define('SIZE',4);
define('CALLERNAME',5);
define('CALLERADDR',6);

if($argc < 2) {
	printf("Usage: %s <file>\n",$argv[0]);
	return 1;
}

$allocs = array();
$matches = array();
$content = implode('',file($argv[1]));
preg_match_all('/\[(A|F)\] p=(\d+) a=([\da-f]+) s=(\d+) c=([^ ]+) \(([\da-f]+)\)/',$content,$matches);

foreach($matches[0] as $k => $v) {
	$addr = $matches[ADDRESS][$k];
	if(!isset($allocs[$addr]))
		$allocs[$addr] = array(0,0,0,'',0);
	if($matches[TYPE][$k] == 'A') {
		$allocs[$addr][0]++;
		$allocs[$addr][1] = $matches[SIZE][$k];
		$allocs[$addr][2] = $matches[PID][$k];
		$allocs[$addr][3] = $matches[CALLERNAME][$k];
		$allocs[$addr][4] = $matches[CALLERADDR][$k];
	}
	else
		$allocs[$addr][0]--;
}

foreach($allocs as $addr => $open) {
	if($open[0] > 0)
		echo $addr.' => '.$open[0].' (size='.$open[1].', pid='.$open[2].', caller='.$open[3].':'.$open[4].')'."\n";
}
?>
