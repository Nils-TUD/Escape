function fak begin
	$res := 1;
	$c := $args[1];
	while($c > 1) do
		$res := $res * $c;
    $c := $c - 1;
	done;
	echo $res;
end;
