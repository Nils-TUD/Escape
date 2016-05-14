#!/usr/bin/php
<?php
$filter = array('vid','cache','sll','util','boot');
if($argc < 2) {
	echo "Usage: $argv[0] <dir>\n";
	exit(1);
}

$edges = array();

echo "digraph ModuleDeps {\n";
echo "	mindist=30.0;\n";
$files = getFiles($argv[1]);
foreach($files as $file)
	parseFile($edges,$file);
echo "}\n";

function parseFile(&$edges,$file) {
	global $filter;
	$c = file_get_contents($file);
	if(!preg_match('/[^#]\b([a-z_][A-Za-z0-9_]+)\s+\**([a-z_][A-Za-z0-9_]+)\([^\)]+?\)/',$c,$match)) {
		file_put_contents('php://stderr',"Unable to find function in file '".$file."'\n");
		return;
	}
	$modname = getModName($match[2]);
	preg_match_all('/(..)\b([a-z]+_[a-zA-Z0-9]{2,})\(/s',$c,$matches);
	foreach($matches[1] as $k => $m) {
		if(!($m[1] == '*' || preg_match('/^[a-zA-Z0-9_]\s$/',$m))) {
			$othermod = getModName($matches[2][$k]);
			if($othermod != $modname && !isset($edges[$modname.'->'.$othermod]) &&
					array_search($othermod,$filter) === false) {
				echo "\t".$modname.'->'.$othermod.";\n";
				$edges[$modname.'->'.$othermod] = true;
			}
		}
	}
}

function getModName($func) {
	$p = strrpos($func,'_');
	if($p === false)
		return $func;
	return substr($func,0,$p);
}

function getFiles($dir) {
	$files = array();
	if(is_dir($dir)) {
		$d = opendir($dir);
		if($d) {
			while($f = readdirto($d)) {
				if($f == '.' || $f == '..' || preg_match('/^\.svn$/',$f))
					continue;
				foreach(getFiles($dir.'/'.$f) as $fd)
					$files[] = $fd;
			}
			closedir($d);
		}
	}
	else if(preg_match('/\.c$/',$dir))
		$files[] = $dir;
	return $files;
}
?>
