#!/bin/bash
# Quick fix for server stunnel config - adds verifyChain=no and verifyPeer=no

sudo bash -c 'cat > /etc/helloworld/stunnel.conf << "STUNNEL_EOF"
; HelloWorld Stunnel Configuration
pid = /var/run/stunnel4/stunnel.pid
setuid = stunnel4
setgid = stunnel4
debug = 0
output = /var/log/helloworld-stunnel.log
sslVersion = all
options = NO_SSLv2
options = NO_SSLv3
ciphers = HIGH:!aNULL:!MD5:!RC4
verifyChain = no
verifyPeer = no
[ssh-tunnel]
accept = 0.0.0.0:443
connect = 127.0.0.1:22
cert = /etc/helloworld/server.pem
key = /etc/helloworld/server.key
STUNNEL_EOF
'
sudo systemctl restart helloworld
echo "Stunnel config updated and service restarted!"

