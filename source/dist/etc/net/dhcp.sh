if($args.len < 3) then
	echo "Usage: ($args[0]) <link> <driver>"
else
	net-links add ($args[1]) ($args[2]);
	net-links up ($args[1]);
	dhcp ($args[1]);
fi
