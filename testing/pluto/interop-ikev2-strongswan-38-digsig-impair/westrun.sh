strongswan up westnet-eastnet-ikev2
ping -n -c 4 -I 192.0.1.254 192.0.2.254

# hash algorithm notification should not be  received due to the impair
grep SIGNATURE_HASH_ALGO /tmp/charon.log | cut -f 2 -d "]" 

echo done
