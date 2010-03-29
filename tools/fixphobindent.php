<?php
error_reporting(E_ALL);
if($argc < 2)
	exit("Usage: php $argv[0] <file>\n");

fixDir($argv[1]);

function fixDir($dir) {
	if(is_dir($dir)) {
		if($d = opendir($dir)) {
			while($f = readdir($d)) {
				if($f == '.' || $f == '..')
					continue;
				fixDir($dir.'/'.$f);
			}
			closedir($d);
		}
	}
	else if(preg_match('/\.d$/',$dir)) {
		$c = file_get_contents($dir);
		if(strpos($c,'    ') !== false) {
			$c = str_replace("\t","\t\t",$c);
			$c = str_replace("    ","\t",$c);
			file_put_contents($dir,$c);
		}
	}
}
?>