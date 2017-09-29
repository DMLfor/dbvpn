//
// Created by dblank on 17-9-14.
//

#include "dbvpnserver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <assert.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <iostream>
#include <tins/tins.h>

using namespace Tins;

VpnServer::VpnServer(const char *tunIp, const char *vpnPort, std::pair<int, int> portRange):
        sender_("eth0"), tunIp_(tunIp), portRange_(portRange)
{
    get_local_ip();
    create_tun();
    create_sock(vpnPort);
    create_epoll();
}

void VpnServer::get_local_ip()
{
    int inet_sock;
    struct ifreq ifr;
    char ip[32] = "";

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, "eth0");
    ioctl(inet_sock, SIOCGIFADDR, &ifr);
    strcpy(ip, inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr));
    localIp_ = IPv4Address(std::string(ip));
}

void VpnServer::create_tun()
{
    struct ifreq ifr;

    err_ = 0;

    tunFd_ = open("/dev/net/tun", O_RDWR);
    assert(tunFd_ != -1);

    bzero(&ifr, sizeof(ifr));
    ifr.ifr_flags |= IFF_TUN | IFF_NO_PI;


    err_ = ioctl(tunFd_, TUNSETIFF, (void *)&ifr);
    assert(err_  != -1);

    tunName_ = std::string(ifr.ifr_name);

    std::string cmd = "";
    cmd += "ifconfig " + tunName_;
    cmd += " " + std::string("192.168.1.0/24") + " up";
    system(cmd.c_str());
}
void VpnServer::create_sock(const char *port)
{
    int sockfd = -1;
    struct sockaddr_in vpnAddr;

    bzero(&vpnAddr, sizeof(vpnAddr));
    vpnAddr.sin_family = AF_INET;
    vpnAddr.sin_port = htons(atoi(port));
    vpnAddr.sin_addr.s_addr = htons(INADDR_ANY);
    vpnAddr_ = vpnAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    assert(sockfd != -1);

    err_ = bind(sockfd, (struct sockaddr *)&vpnAddr, sizeof(vpnAddr));
    assert(err_ != -1);

    sockFd_ = sockfd;
}

void VpnServer::create_epoll()
{
    epFd_ = epoll_create1(0);

    assert(epFd_ != -1);

    epoll_add(sockFd_, EPOLLIN);
    epoll_add(tunFd_, EPOLLIN);

}

void VpnServer::epoll_add(int fd, int status)
{
    struct epoll_event ev;
    ev.events = status;
    ev.data.fd = fd;
    err_ = epoll_ctl(epFd_, EPOLL_CTL_ADD, fd, &ev);

    assert(err_ != -1);
}

bool VpnServer::get_ip_pack(const uint8_t *buf, int n, IP &pack)
{
   try
   {
		pack = RawPDU(buf, n).to<IP>();
   }
   catch (...)
   {
	   return false;
   }
   return true;
}
void VpnServer::start()
{
    std::cout << "The " << getTunNanme() << " is running." << std::endl;
    std::cout << "Port range is " << portRange_.first << "-" << portRange_.second << std::endl;
    time_t nowTime = time(NULL);

    for(int i = portRange_.first+1; i<portRange_.second; i++)
        portArr_.push_back(Port(i));

    uint8_t buf[BUFF_SIZE];
    struct sockaddr_in tmpAddr;
    socklen_t sockLen = sizeof(struct sockaddr);
    long long cnt = 0;
    while(1)
    {
        int nfds = epoll_wait(epFd_, evList_, EPOLL_SIZE, -1);
        for(int i = 0; i<nfds; i++)
        {
            if(evList_[i].data.fd == tunFd_)
            {
                int nread = read(tunFd_, buf, sizeof(buf));

				IP oneIpPack;

				if(get_ip_pack(buf, nread, oneIpPack) == false)
					continue;

                DNAT(oneIpPack);
            }
            else if(evList_[i].data.fd == sockFd_)
            {
                int nrecv =  recvfrom(sockFd_, buf, sizeof(buf), 0, (struct sockaddr *)&tmpAddr, &sockLen);

                IP oneIpPack;

				if(get_ip_pack(buf, nrecv, oneIpPack) == false)
					continue;

                SNAT(oneIpPack, tmpAddr);
            }
        }
    }
}

void VpnServer::modPort(Tins::IP &pack, uint16_t value, int type)
{
    if(pack.protocol() == IPPROTO_TCP && type == MOD_DST)
    {
        TCP *oneTcp = pack.find_pdu<TCP>();
        oneTcp->dport(value);
    }
    if(pack.protocol() == IPPROTO_UDP && type == MOD_DST)
    {
        UDP *oneUdp = pack.find_pdu<UDP>();
        oneUdp->dport(value);
    }
    if(pack.protocol() == IPPROTO_TCP && type == MOD_SRC)
    {
        TCP *oneTcp = pack.find_pdu<TCP>();
        oneTcp->sport(value);
    }
    if(pack.protocol() == IPPROTO_UDP && type == MOD_SRC)
    {
        UDP *oneUdp = pack.find_pdu<UDP>();
        oneUdp->sport(value);
    }

}
void VpnServer::get_quintet(const Tins::IP &IpPack, Quintet &quintet)
{
    quintet.dstIP = IpPack.dst_addr(), quintet.srcIp = IpPack.src_addr();
    quintet.protocol = IpPack.protocol();
    if(IpPack.protocol() == IPPROTO_UDP)
    {
        const UDP *oneUdp = IpPack.find_pdu<UDP>();
        quintet.dstPort = oneUdp->dport();
        quintet.srcPort = oneUdp->sport();
    }
    else if(IpPack.protocol() == IPPROTO_TCP)
    {
        const TCP *oneTcp = IpPack.find_pdu<TCP>();
        quintet.dstPort = oneTcp->dport();
        quintet.srcPort = oneTcp->sport();
    }
    else if(IpPack.protocol() == IPPROTO_ICMP)
    {
        return ;
    }
}
void VpnServer::write_tun(Tins::PDU &ip)
{
    char buf[BUFF_SIZE];
    std::vector<uint8_t> serV = ip.serialize();
    for(int i = 0; i<serV.size(); i++)
        buf[i] = serV[i];

    socklen_t sockLen = sizeof(struct sockaddr_in);

    write(tunFd_, buf, serV.size());
}

void VpnServer::SNAT(const Tins::IP &IpPack, const struct sockaddr_in &clientAddr)
{
    time_t nowTime = time(nullptr);
    IP oneIpPack = IpPack;
    Quintet quintet;
    get_quintet(IpPack, quintet);
    IpPort dstIpPort = std::make_pair(quintet.dstIP, quintet.dstPort);
    IpPort srcIpPort = std::make_pair(quintet.srcIp, quintet.srcPort);

    //printQuintet(quintet);
    if(addrToPort_.count(srcIpPort) == 0)
    {
        for(auto &item : portArr_)
        {
            if(nowTime - item.lastTime >= 300 || item.used == false)
            {
                if(nowTime - item.lastTime >= 300)
                {
                    IpPort tmp = portToAddr_[item.port];
                    addrToPort_.erase(tmp);
                    portToAddr_.erase(item.port);
                    simpleSockaddr_.erase(tmp);
                }

                item.lastTime = nowTime;
                item.used = true;
                modPort(oneIpPack, item.port, MOD_DST);
                addrToPort_[srcIpPort] = item.port;
                portToAddr_[item.port] = srcIpPort;
                simpleSockaddr_[srcIpPort] = clientAddr;
                srcIpPort.second = item.port;
                break;
            }
        }
    }
    else
    {
        uint16_t tmp = addrToPort_[srcIpPort];
        int seq = tmp - portRange_.first -1;
        portArr_[seq].lastTime = nowTime;
        modPort(oneIpPack, tmp, 0);
    }

    oneIpPack.src_addr(tunIp_);
    write_tun(oneIpPack);
}

void VpnServer::DNAT(const Tins::IP &IpPack)
{
    char buf[BUFF_SIZE];
    IP oneIpPack = IpPack;
    Quintet quintet;
    get_quintet(IpPack, quintet);
    IpPort srcIpPort = std::make_pair(quintet.srcIp, quintet.srcPort);
    if(portToAddr_.count(quintet.dstPort) == 0)
    {
        return ;
    }
    IpPort dstIpPort = portToAddr_[quintet.dstPort];
    struct sockaddr_in tmpAddr = simpleSockaddr_[dstIpPort];

    oneIpPack.dst_addr(dstIpPort.first);

    modPort(oneIpPack, dstIpPort.second, MOD_SRC);

    std::vector<uint8_t> serV = oneIpPack.serialize();

    for (int i = 0; i < serV.size(); i++)
        buf[i] = serV[i];
    get_quintet(oneIpPack, quintet);
    socklen_t sockLen = sizeof(struct sockaddr_in);
    sendto(sockFd_, buf, serV.size(), 0,(struct sockaddr *)&tmpAddr, sockLen);
}
