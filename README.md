# dbvpn

A sipmle VPN implementation based on Linux Tun device 

一个简单的VPN 实现使用TUN设备

# Compile and Run

cmake 3.8

### client
```
mkdir build && cd build
cmake ../
make

./dbvpnclient server_ip server_port 

```

### server 

```
mkdir build && cd build
cmake ../
make

./dbvpnclient tun_ip server_port port_snat_range

```

# Libraries used

 [libtins](https://github.com/mfontanini/libtins)

 Mainly used lintins to make changes to packages

# More

[littleblank](http://www.littleblank.net/archives/1067/)

