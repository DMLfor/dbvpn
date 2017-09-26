#include "dbvpnserver.h"

#include <iostream>

using namespace std;

int main(int argv, char *argc[])
{

    if (argv > 2)
    {
        VpnServer server(argc[1], argc[2]);
        server.start();
    }
    else
    {
        printf("Input IP Port!\n");
    }
    while (1);
    return 0;
}
