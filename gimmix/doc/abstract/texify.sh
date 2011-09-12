#!/bin/bash
pdflatex -file-line-error -file-line-error-style $@ | grep -A 3 '^[^:]*:[0-9]\+' | \
	php -B '$usenext = false;' -R '
  if($usenext || preg_match("/^[^:]+:\d+:/",$argn)) {
  	$usenext = strlen($argn) == 79;
  	echo $argn;
  	if(!$usenext)
  		echo "\n";
  }'
