#include <iostream>
#include "server.h"

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        std::cout << "requre 1 arg - port\n";
        return 0;
    }

    CServer Server = CServer(atoi(argv[1]));
    return Server.start();
    return 0;
}

