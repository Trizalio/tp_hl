#include <iostream>
#include "server.h"

int main(int argc, char *argv[])
{

    /*std::cout << argc << "\n";
    for(int i = 0; i < argc; ++i)
    {
        std::cout << argv[i];
        if(i+1 < argc)
        {
            std::cout << ", ";
        }
    }
    std::cout << "\n";*/
    if(argc != 2)
    {
        std::cout << "requre 1 arg - port\n";
        return 0;
    }

    CServer Server = CServer(atoi(argv[1]));
    return Server.start();
    return 0;
}

