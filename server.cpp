#include "server.h"
#include "httplogic.h"


void threadListener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
void connectionWrite_cb(struct bufferevent *, void *);
void connectionRead_cb(struct bufferevent *, void *);
void connectionEvent_cb(struct bufferevent *, short, void *);
void connectionError_cb(struct evconnlistener *listener, void *ctx);

////mulithreading
//static void theadFunc1();
boost::lockfree::queue<evutil_socket_t> queue(1024);


void mainListner_cb(struct evconnlistener *listener, evutil_socket_t fd,
        struct sockaddr *sa, int socklen, void *user_data)
{
    //std::cout << "mainListner_cb " << std::this_thread::get_id() <<"\n";
    queue.push(fd);
}


static void threadLooper() {
    std::cout << "threadLooper " << std::this_thread::get_id() <<"\n";
    event_base *base = event_base_new();
    if (!base) {
        std::cout << "!base";
    }
    evutil_socket_t fd;
    while(1) {
        if(queue.pop(fd)){
            threadListener_cb(NULL, fd, NULL, 0, (void*)base);
        }else{
            usleep(1);
        }
        event_base_loop(base, EVLOOP_ONCE);
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

    listener = evconnlistener_new_bind(base, mainListner_cb, (void *)base,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin,
        sizeof(sin));

    std::cout << "Starting server on localhost:"<< port <<"\nNumber of working threads:"<< numThreads <<"\n";
    for(int i = 0; i < numThreads; ++i) {
        std::thread (threadLooper).detach();
    }

    evconnlistener_set_error_cb(listener, connectionError_cb);
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

void threadListener_cb(struct evconnlistener *listener, evutil_socket_t fd,
        struct sockaddr *sa, int socklen, void *user_data)
{
    //std::cout << "threadListener_cb " << std::this_thread::get_id() <<"\n";
    event_base *base = (event_base*)user_data;
    bufferevent *bufferevent = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if(!bufferevent) {
        std::cout << "!base";
    }
    bufferevent_setcb(bufferevent, connectionRead_cb, NULL, connectionEvent_cb, NULL);
    if (bufferevent_enable(bufferevent, EV_READ) == -1){
        std::cout << "bufferevent_enable(bev, EV_READ) == -1";
    }
}

void connectionWrite_cb(struct bufferevent *pBufferEvent, void *user_data)
{
    //std::cout << "connectionWrite_cb " << std::this_thread::get_id() <<"\n";
    struct evbuffer *output = bufferevent_get_output(pBufferEvent);
    if (evbuffer_get_length(output) == 0 && BEV_EVENT_EOF ) {
        //std::cout << "flushed answer \n";
        bufferevent_free(pBufferEvent);
    }
}

void connectionRead_cb(struct bufferevent *pBufferEvent, void *user_data)
{
    //std::cout << "connectionRead_cb " << std::this_thread::get_id() <<"\n";
    createResponse(pBufferEvent);

    //Setup write callback
    bufferevent_setcb(pBufferEvent, NULL, connectionWrite_cb, connectionEvent_cb, NULL);
    bufferevent_enable(pBufferEvent, EV_WRITE);
}

void connectionEvent_cb(struct bufferevent *pBufferEvent, short events, void *user_data)
{
    //std::cout << "connectionEvent_cb" << std::this_thread::get_id() <<"\n";
    /*if (events & BEV_EVENT_ERROR)
            std::cout << "Error from bufferevent "<< evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()) <<"\n";*/
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
            bufferevent_free(pBufferEvent);
    }
}


void connectionError_cb(struct evconnlistener *listener, void *ctx)
{
        //std::cout << "connectionError_cb " << std::this_thread::get_id() <<"\n";
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();\
        std::cout << "Got an error " << err << ": " << evutil_socket_error_to_string(err);

        event_base_loopexit(base, NULL);
}

