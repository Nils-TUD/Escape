<?php
error_reporting(E_ALL);
if($argc < 2) {
	printf("Usage: %s <file>\n",$argv[0]);
	return 1;
}

$locks = array();
$matches = array();
$content = file_get_contents($argv[1]);
preg_match_all('/\[(\d+)\] (L|U) 0x([a-fA-F0-9]+) ([A-Fa-f0-9 ]+)/s',$content,$matches);

foreach($matches[0] as $k => $v) {
	$tid = $matches[1][$k];
	$type = $matches[2][$k];
	$lock = $matches[3][$k];
	$trace = $matches[4][$k];
	
	if(!isset($locks[$lock]))
		$locks[$lock] = array(0,0,'');
	if($type == 'L') {
		if($locks[$lock][0] != 0)
			echo "ERROR: multiple locks!?\n";
		$locks[$lock][0]++;
		$locks[$lock][1] = $tid;
		$locks[$lock][2] = $trace;
	}
	else {
		if($locks[$lock][0] != 1)
			echo "ERROR: multiple locks!?\n";
		$locks[$lock][0]--;
	}
}

foreach($locks as $lock => $open) {
	if($open[0] != 0)
		echo $lock.' => '.$open[0].' (caller='.$open[1].'; '.strtolower($open[2]).')'."\n";
}
?>
