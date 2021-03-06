#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <assert.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <cstring>

class CServer
{
public:
    CServer(unsigned int port);
    int start();
private:

    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;
};

#endif // SERVER_H
