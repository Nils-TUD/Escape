<?php
if($argc != 3)
	die('Usage: '.$argv[0].' <file> <symbolfile>'."\n");

function resolve($addr,$symbolfile) {
	$res = array();
	exec('readelf -sW '.escapeshellarg($symbolfile).' | grep '.escapeshellarg($addr).' | xargs | cut -d \' \' -f 8',$res);
	return implode('',$res);
}

$c = file_get_contents($argv[1]);
echo preg_replace('/CALL ([a-f0-9]+)/e','"CALL ".resolve("\1","'.$argv[2].'")." (\1)"',$c);
?>
