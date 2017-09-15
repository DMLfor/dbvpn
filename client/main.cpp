#include "dbvpnclient.h"

#include <iostream>

using namespace std;

int main(int argv, char *argc[])
{

    if (argv > 2)
    {
        TunDev tun(argc[1], argc[2]);

        cout << "The " << tun.getTunNanme() << " is running." << endl;

        tun.start();

    }
    else
    {
        printf("Input IP Port!\n");
    }
    while (1);
    return 0;
}
