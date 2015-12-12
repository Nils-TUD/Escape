#!/bin/sh

# based on:
# http://askubuntu.com/questions/11709/how-can-i-capture-network-traffic-of-a-single-process

hostif=enp2s0
hostip=`ip addr show enp2s0 | grep -Po "inet \K[\d.]+"`

usage() {
	echo "Usage: $1 (create|remove|exec) [...]" >&2
	exit 1
}

if [ $# -lt 1 ]; then
	usage $0
fi

cmd=$1
shift

case $cmd in
	create)
		# create net namespace
		ip netns add test || exit 1
		# create new device in that ns and in the default one
		ip link add veth-a type veth peer name veth-b
		ip link set veth-a netns test

		# set IP addresses and up them
		ip netns exec test ip addr add dev veth-a local 192.168.163.1/24
		ip netns exec test ip link set dev veth-a up
		ip addr add dev veth-b local 192.168.163.254/24
		ip link set dev veth-b up

		# let veth-a route everything to veth-b
		ip netns exec test ip route add dev veth-a default via 192.168.163.254

		# enable forwarding to host IP
		echo 1 > /proc/sys/net/ipv4/ip_forward
		iptables -t nat -A POSTROUTING -s 192.168.163.0/24 -o $hostif -j SNAT --to-source $hostip
		;;

	remove)
		ip netns delete test || exit 1
		echo 0 > /proc/sys/net/ipv4/ip_forward
		iptables -t nat -D POSTROUTING -s 192.168.163.0/24 -o $hostif -j SNAT --to-source $hostip
		;;

	exec)
		ip netns exec test $@
		;;

	*)
		usage $0
		;;
esac
