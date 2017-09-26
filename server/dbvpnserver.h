//
// Created by dblank on 17-9-14.
//

#ifndef DBVPN_DBVPNCLIENT_H
#define DBVPN_DBVPNCLIENT_H

#define IP_SIZE 128
#define IF_NAME_SIZE 16
#define BUFF_SIZE 2048         //must >= pack size
#define EPOLL_SIZE 16
#include <string>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <tins/tins.h>
#include <iostream>
class VpnServer
{
private:
    typedef std::pair<Tins::IPv4Address, uint16_t > IpPort;
    typedef std::map<IpPort, IpPort> SNat;
    typedef std::map<IpPort, struct sockaddr_in> IpPortToSockaddr;
    typedef struct Quintet_t
    {
        Tins::IPv4Address srcIp, dstIP;
        uint16_t srcPort, dstPort;
        int protocol;
        Quintet_t(){}
        Quintet_t(const VpnServer::Quintet_t &quintet_)
        {
            *this = quintet_;
        }
        Quintet_t(Tins::IPv4Address srcIp_, uint16_t srcPort_, Tins::IPv4Address dstIp_, uint16_t dstPort_, int protocol_)
        {
            srcIp = srcIp_, srcPort = srcPort_;
            dstIp_ = dstIp_, dstPort_ = dstPort_;
            protocol = protocol_;
        }
    } Quintet;

    int tunFd_;
    int err_;
    std::string tunName_;
    int sockFd_;
    int epFd_;
    struct sockaddr_in vpnAddr_, clientAddr_;
    struct epoll_event evList_[EPOLL_SIZE];
    Tins::PacketSender sender_;
    Tins::IPv4Address localIp_, tunIp_;
    IpPortToSockaddr simpleSockaddr_;
    SNat simpleNat_;

    void create_sock(const char *port);

    void create_tun(const char *tunIp);

    void create_epoll();

    void get_local_ip();

    void epoll_add(int fd, int status);

    void SNAT(const Tins::IP &IpPack, const struct sockaddr_in &clientAddr);

    void DNAT(const Tins::IP &IpPack);

    void write_tun(Tins::PDU &ip);

    void get_quintet(const Tins::IP &IpPack, Quintet &quintet);

    void printQuintet(const Quintet & quintet)
    {
        if(quintet.protocol == IPPROTO_TCP)
           std::cout << "TCP :";
        else if(quintet.protocol == IPPROTO_UDP)
           std::cout << "UDP :";
        std::cout << "src : " << quintet.srcIp.to_string() << ":" << ntohs(quintet.srcPort) << " -> "
                    << quintet.dstIP.to_string() << ":" << ntohs(quintet.dstPort)<<std::endl;
    }

public:

    VpnServer(const char *tunIp, const char *vpnPort);

    void start();

    std::string getTunNanme() const
    {
        return tunName_;
    }

};



#endif //DBVPN_DBVPNCLIENT_H
