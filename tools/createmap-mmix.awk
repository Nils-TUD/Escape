<?php
$lines = file('php://stdin');
foreach($lines as $line) {
	if(preg_match('/^([0-9A-Fa-f]+) (t|T) (.*?)$/',$line,$match))
		printf("%s\tCODE 0x%s\tsize 0x0\n",$match[3],$match[1]);
}
printf("\n")
printf("CODE\tstart\t0x0\n")
?>
