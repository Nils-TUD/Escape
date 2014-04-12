net-links add lo /sbin/lo;
net-links set lo ip 127.0.0.1;
net-links set lo subnet 255.0.0.0;
net-links up lo;
net-routes add lo 127.0.0.1 255.0.0.0 0.0.0.0;
