#!/usr/bin/php
<?php
$data = file_get_contents('php://stdin');
preg_match_all('/(\d+):(\d+) (\S+) ([0-9a-f]+) ([0-9a-f]+)(?: \((\d+)\))?/',$data,$matches);

$req = array();

foreach($matches[0] as $k => $dummy) {
	switch($matches[3][$k]) {
		case 'roreq':
		case 'rsreq':
		case 'rrreq':
		case 'rwreq':
		case 'rpreq':
		case 'oreq':
		case 'rreq':
		case 'wreq':
			$req[] = array(
				$matches[0][$k],$matches[1][$k],$matches[2][$k],$matches[4][$k],$matches[5][$k]
			);
			break;
		
		case 'rores':
		case 'rsres':
		case 'rrres':
		case 'rdres':
		case 'ores':
		case 'rres':
		case 'wres':
			foreach($req as $i => $r) {
				if($r[3] == $matches[4][$k]) {
					if($r[2] != $matches[6][$k]) {
						echo "ERROR:\n";
						echo "  Request: ".$r[0]."\n";
						echo "  Response: ".$matches[0][$k]."\n";
						print_r($req);
						echo "\n";
					}
					break;
				}
			}
			break;
		
		case 'rem':
			foreach($req as $i => $r) {
				if($r[4] == $matches[5][$k])
					unset($req[$i]);
			}
			break;
		
		default:
			echo "UNKNOWN?!?\n";
			break;
	}
}
?>
