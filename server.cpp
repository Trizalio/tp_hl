#include "server.h"
#include "httplogic.h"


void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
void conn_writecb(struct bufferevent *, void *);
void conn_readcb(struct bufferevent *, void *);
void conn_eventcb(struct bufferevent *, short, void *);
void accept_error_cb(struct evconnlistener *listener, void *ctx);

CServer::CServer(unsigned int port)
{

    std::cout << "init with port " << port << "\n";


    base = event_base_new();
    if (!base) {
        std::cout << "Could not initialize libevent!\n";
        assert(0);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;

    listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin,
        sizeof(sin));

    if (!listener) {
        std::cout << "Could not create a listener!\n";
        assert(0);
    }

    evconnlistener_set_error_cb(listener, accept_error_cb);




}
int CServer::start()
{

    std::cout << "attempt to start\n";


    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_base_free(base);

    std::cout << "Bye!\n";

    return 0;
}


void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
    struct event_base *base = (event_base*)user_data;
    struct bufferevent *bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        std::cout << "Error constructing bufferevent!\n";
        event_base_loopbreak(base);
        return;
    }
    bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, NULL);
    int res = bufferevent_enable(bev, EV_READ);
}


void conn_writecb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0 && BEV_EVENT_EOF ) {
        //std::cout << "flushed answer \n";
        bufferevent_free(bev);
    }
}

void conn_readcb(struct bufferevent *pBufferEvent, void *user_data)
{
    createResponse(pBufferEvent);

    //Setup write callback
    bufferevent_setcb(pBufferEvent, NULL, conn_writecb, conn_eventcb, NULL);
    bufferevent_enable(pBufferEvent, EV_WRITE);
}

void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    //std::cout << "event callback\n";
    if (events & BEV_EVENT_ERROR)
            std::cout << "Error from bufferevent";
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
            bufferevent_free(bev);
    }
}


void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();\
        std::cout << "Got an error " << err << ": " << evutil_socket_error_to_string(err);

        event_base_loopexit(base, NULL);
}
