if($args.len < 3) then
	echo "Usage: ($args[0]) <link> <driver>"
else
	net-links add ($args[1]) ($args[2]);
	net-links set ($args[1]) ip 192.168.2.110;
	net-links set ($args[1]) subnet 255.255.255.0;
	net-links up ($args[1]);
	net-routes add ($args[1]) 192.168.2.0 255.255.255.0 0.0.0.0;
	net-routes add ($args[1]) default 0.0.0.0 192.168.2.1;
fi
