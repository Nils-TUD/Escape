net-links add eth0 /sbin/e1000;
net-links set eth0 ip 192.168.2.110;
net-links set eth0 subnet 255.255.255.0;
net-links up eth0;
net-routes add eth0 192.168.2.0 255.255.255.0 0.0.0.0;
net-routes add eth0 default 0.0.0.0 192.168.2.1;
