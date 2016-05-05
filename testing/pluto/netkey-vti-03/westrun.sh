ipsec auto --up  westnet-eastnet-vti-01
ipsec auto --up  westnet-eastnet-vti-02
ip tunnel add name ipsec0 mode vti local 192.1.2.45 remote 0.0.0.0 ikey 2 okey 1  dev eth0
#ip tunnel add name ipsec0 mode vti local 192.1.2.45 remote 0.0.0.0 ikey 2 okey 1  dev eth1
ip link set ipsec0 up
sysctl -w net.ipv4.conf.ipsec0.disable_policy=1
sysctl -w net.ipv4.conf.ipsec0.disable_xfrm=1
sysctl -w net.ipv4.conf.ipsec0.rp_filter=0
sysctl -w net.ipv4.conf.ipsec0.forwarding=1
#ip addr add 192.0.1.0/24 remote 192.0.2.0/24 dev ipsec0
#ip addr add 10.0.1.0/24 remote 10.0.2.0/24 dev ipsec0
ip route add 192.0.2.0/24 dev ipsec0
ip route add 10.0.2.0/24 dev ipsec0
ping -n -c 4 -I 192.0.1.254 192.0.2.254
ping -n -c 4 -I 10.0.1.254 10.0.2.254
ipsec whack --trafficstatus
echo done
