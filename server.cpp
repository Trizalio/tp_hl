#include "server.h"
#include "httplogic.h"


void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
void conn_writecb(struct bufferevent *, void *);
void conn_readcb(struct bufferevent *, void *);
void conn_eventcb(struct bufferevent *, short, void *);
void accept_error_cb(struct evconnlistener *listener, void *ctx);

////mulithreading
static void theadFunc1();
boost::lockfree::queue<int> q(1024);


void listener_cb_1(struct evconnlistener *listener, evutil_socket_t fd,
        struct sockaddr *sa, int socklen, void *user_data)
{
    //std::cout << "listener_cb_1 " << std::this_thread::get_id() <<"\n";
    q.push(fd);
}

static void threadFunc1() {
    std::cout << "threadFunc1 " << std::this_thread::get_id() <<"\n";
    event_base *base = event_base_new();
    if (!base) {
        perror("Base new");
    }
    int fd;
    while(1) {
        event_base_loop(base, EVLOOP_ONCE);
        if(q.pop(fd)){
            listener_cb(NULL, fd, NULL, 0, (void*)base);
        }
    }
}

CServer::CServer(unsigned int port)
{

    std::cout << "init with port " << port << "\n";

    ////mulithreading
    isRun = true;
    int numThreads = std::thread::hardware_concurrency();

    base = event_base_new();
    if (!base) {
        std::cout << "Could not initialize libevent!\n";
        assert(0);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;

    listener = evconnlistener_new_bind(base, listener_cb_1, (void *)base,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin,
        sizeof(sin));

    std::cout << "Starting server on localhost:"<< port <<"\nNumber of working threads:"<< numThreads <<"\n";
    for(int i = 0; i < numThreads; ++i) {
        std::thread (threadFunc1).detach();
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

void threadFunc(int fd, struct event_base *base) {
    std::cout << "threadFunc " << std::this_thread::get_id() <<"\n";
    struct bufferevent *bev;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }
    bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, NULL);
    bufferevent_enable(bev, EV_READ);
    //bufferevent_setcb(ev, conn_readcb, NULL, conn_eventcb, NULL);
}

////mulithreading
void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
        struct sockaddr *sa, int socklen, void *user_data)
{
    //std::cout << "listener_cb " << std::this_thread::get_id() <<"\n";
    event_base *base = (event_base*)user_data;
    bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if(!bev) {
        perror("Bev");
    }
    bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, NULL);
    if (bufferevent_enable(bev, EV_READ) == -1){
        perror("Read");
    }
}

/*void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
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
}*/

void conn_writecb(struct bufferevent *bev, void *user_data)
{
    //std::cout << "conn_writecb " << std::this_thread::get_id() <<"\n";
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0 && BEV_EVENT_EOF ) {
        //std::cout << "flushed answer \n";
        bufferevent_free(bev);
    }
}

void conn_readcb(struct bufferevent *pBufferEvent, void *user_data)
{
    //std::cout << "conn_readcb callback " << std::this_thread::get_id() <<"\n";
    createResponse(pBufferEvent);

    //Setup write callback
    bufferevent_setcb(pBufferEvent, NULL, conn_writecb, conn_eventcb, NULL);
    bufferevent_enable(pBufferEvent, EV_WRITE);
}

void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    //std::cout << "conn_eventcb" << std::this_thread::get_id() <<"\n";
    if (events & BEV_EVENT_ERROR)
            std::cout << "Error from bufferevent "<< evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()) <<"\n";
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
            bufferevent_free(bev);
    }
}


void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        //std::cout << "accept_error_cb " << std::this_thread::get_id() <<"\n";
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();\
        std::cout << "Got an error " << err << ": " << evutil_socket_error_to_string(err);

        event_base_loopexit(base, NULL);
}

