# dbvpn
一个简单的VPN 实现使用TUN设备

# 编译运行

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

./dbvpnclient tun_ip server_port 

```

# 使用到的库

使用到 [libtins](https://github.com/mfontanini/libtins), 主要是用来进行一个包的修改.

# 更多

[littleblank](http://www.littleblank.net/archives/1067/)

