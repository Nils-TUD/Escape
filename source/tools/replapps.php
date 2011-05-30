<?php
if($argc != 3) {
	echo "Usage: $argv[0] <src> <disk>\n";
	exit(1);
}

$apps = file_get_contents($argv[1]);
$apps = explode(';;',$apps);
$app2ino = array();
$var = `ls -i -1 --color=never $argv[2]/sbin $argv[2]/bin`;
foreach(explode("\n",$var) as $line) {
	$matches = array();
	if(preg_match('/^\s*(\d+)\s+(\S+)$/',$line,$matches))
		$app2ino[$matches[2]] = $matches[1];
}

foreach($apps as $k => $app) {
	preg_match('/name:\s*"(.*?)";/',$app,$matches);
	$apps[$k] = preg_replace(
		'/inodeNo:\s*\d*;/',
		'inodeNo:					' . (string)$app2ino[$matches[1]] . ';',
		$app
	);
}

file_put_contents($argv[1],implode(';;',$apps));
?>
