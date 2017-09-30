#include "dbvpnserver.h"

#include <iostream>

void usage()
{
    fprintf(stderr, "Usage:\n\t<tun local ip> (for example: 192.168.1.20)\n \
            \t <the udp socket port> (for example: 30000)\n \
             <the snat port range> (for example: 30000-34999)");
    exit(0);
}
int main(int argc, char *argv[])
{
    if(argc == 4)
    {
        std::pair<int, int> portRange;
        int num = 0;
        for(int i = 0; argv[3][i]; i++)
        {
            if(isdigit(argv[3][i]))
                num = num * 10 + argv[3][i] - '0';
            else if(argv[3][i] == '-')
                portRange.first = num, num = 0;
        }
        portRange.second = num;
        if(!(portRange.first > 1024 && portRange.first < 65536 && portRange.second > 1024 && portRange.second < 65536 && portRange.first > portRange.second))
        {
            usage();
        }
        VpnServer server(argv[1], argv[2], portRange);
        server.start();
    }
    else
    {
        usage();
    }
    return 0;
}
