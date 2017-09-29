//
// Created by dblank on 17-9-14.
//

#include "dbvpnclient.h"

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

TunDev::TunDev(const char *vpnIp, const char *vpnPort):
    vpnIp_(vpnIp)
{
    create_tun();
    create_sock(vpnPort);
    create_epoll();

    bzero(&vpnAddr_, sizeof(vpnAddr_));
    vpnAddr_.sin_family = AF_INET;
    vpnAddr_.sin_port = htons(atoi(vpnPort));
    inet_pton(AF_INET, vpnIp, &vpnAddr_.sin_addr);

}

void TunDev::create_tun()
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
    cmd += "ifconfig " + tunName_ + " mtu 1300" + " up";
    system(cmd.c_str());
    system("route add default dev tun0");
 }

void TunDev::create_sock(const char *port)
{
    int sockfd = -1;

    struct sockaddr_in vpnAddr;

    bzero(&vpnAddr, sizeof(vpnAddr));
    vpnAddr.sin_family = AF_INET;
    vpnAddr.sin_port = htons(atoi(port));
    vpnAddr.sin_addr.s_addr = htons(INADDR_ANY);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    assert(sockfd != -1);

    err_ = bind(sockfd, (struct sockaddr *)&vpnAddr, sizeof(vpnAddr));
    assert(err_ != -1);

    sockFd_ = sockfd;
}

void TunDev::create_epoll()
{
    epFd_ = epoll_create1(0);

    assert(epFd_ != -1);

    epoll_add(sockFd_, EPOLLIN);
    epoll_add(tunFd_, EPOLLIN);

}


void TunDev::epoll_add(int fd, int status)
{
    struct epoll_event ev;
    ev.events = status;
    ev.data.fd = fd;
    err_ = epoll_ctl(epFd_, EPOLL_CTL_ADD, fd, &ev);

    assert(err_ != -1);
}
void TunDev::start()
{
    uint8_t buf[BUFF_SIZE];
    struct sockaddr tmpAddr;
    socklen_t sockLen = sizeof(struct sockaddr);
    while(1)
    {
        int nfds = epoll_wait(epFd_, evList_, EPOLL_SIZE, -1);

        for(int i = 0; i<nfds; i++)
        {
            if(evList_[i].data.fd == tunFd_)
            {
                int nread =  read(tunFd_, buf, sizeof(buf));
                const void *peek = buf;
                const struct iphdr *iphdr = static_cast<const struct iphdr *>(peek);
                if(iphdr->version == 4)
                {
                    sendto(sockFd_, buf, nread, 0, (struct sockaddr *)&vpnAddr_, sockLen);
                }
            }
            else if(evList_[i].data.fd == sockFd_)
            {
                int nrecv =  recvfrom(sockFd_, buf, sizeof(buf), 0, &tmpAddr, &sockLen);
                write(tunFd_, buf, nrecv);
            }
        }
    }
}
