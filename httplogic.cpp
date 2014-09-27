#include "httplogic.h"

void createResponse(bufferevent *pBufferEvent)
{
    struct evbuffer* pInputBuffer = bufferevent_get_input(pBufferEvent);
    struct evbuffer* pOutputBuffer = bufferevent_get_output(pBufferEvent);

    char *line;
    size_t size = 0;
    line = evbuffer_readln(pInputBuffer, &size, EVBUFFER_EOL_CRLF);

    std::string Request = std::string(line);
    //std::cout << Request << "\n";

    //read method and path
    size_t nMethodEnd = Request.find(" ");
    std::string sMethod = std::string(Request, 0, nMethodEnd);
    //std::cout << sMethod << "\n";

    size_t nPathEnd = Request.find(" ", nMethodEnd+1);
    std::string sRawPath = std::string(Request, nMethodEnd+1, nPathEnd-nMethodEnd-1);
    //std::cout << sRawPath << "\n";

    size_t nDelimPos = sRawPath.find('?');
    std::string sHalfRawPath = sRawPath.substr(0, nDelimPos);
    //std::cout << sHalfRawPath << "\n";
    char* sTempPath = new char[sHalfRawPath.length()+1]; // at most the same size + term\0
    urlDecode(sTempPath, sHalfRawPath.c_str());
    std::string sPath = std::string(sTempPath);
    //std::cout << sPath << "\n";

    if (Request.find(" ", nPathEnd+1) != std::string::npos) { // extra spaces in request
        //std::cout << "ERROR: extra spaces\n";
        writeHeader(pOutputBuffer, STATUS_BAD_REQUEST, TYPE_HTML, MASSAGE_LENGTH_BAD_REQUEST);
        evbuffer_add(pOutputBuffer, MASSAGE_BAD_REQUEST, MASSAGE_LENGTH_BAD_REQUEST);
        return;
    }

    //Validate path
    if(!validatePath(sPath))
    {
        //std::cout << "Warning: forbidden path\n";
        writeHeader(pOutputBuffer, STATUS_FORBIDDEN, TYPE_HTML, MASSAGE_LENGTH_FORBIDDEN);
        evbuffer_add(pOutputBuffer, MASSAGE_FORBIDDEN, MASSAGE_LENGTH_FORBIDDEN);
        return;
    }
    //std::cout << "Ok: path ok\n";



    //find target file
    std::string sRealPath = std::string(DOCUMENT_ROOT);
    sRealPath.append(std::string(sPath));

    bool bIndex = false;
    if (sRealPath[sRealPath.length()-1] == '/') {
        //std::cout << "Ok: index\n";
        sRealPath.append("index.html");
        bIndex = true;
    }

    int fFile = open(sRealPath.c_str(), O_NONBLOCK|O_RDONLY);
    if (fFile == -1) {
        //std::cout << "Warning: file not opened\n";
        if (bIndex) {
            writeHeader(pOutputBuffer, STATUS_FORBIDDEN, TYPE_HTML, MASSAGE_LENGTH_FORBIDDEN);
            evbuffer_add(pOutputBuffer, MASSAGE_FORBIDDEN, MASSAGE_LENGTH_FORBIDDEN);
        } else {
            writeHeader(pOutputBuffer, STATUS_NOT_FOUND, TYPE_HTML, MASSAGE_LENGTH_NOT_FOUND);
            evbuffer_add(pOutputBuffer, MASSAGE_NOT_FOUND, MASSAGE_LENGTH_NOT_FOUND);
        }
        return;
    }
    //std::cout << "Ok: file opened\n";

    struct stat FileStats;
    fstat(fFile, &FileStats);
    if(!strcmp(sMethod.c_str(), METHOD_GET)) {
        //std::cout << "Ok: method \"get\"\n";
        writeHeader(pOutputBuffer, STATUS_OK, getContentType(sRealPath), FileStats.st_size);
        evbuffer_add_file(pOutputBuffer, fFile, 0, FileStats.st_size);
    } else if(!strcmp(sMethod.c_str(), METHOD_HEAD)){
        //std::cout << "Ok: method \"head\"\n";
        // ctime gives only /n so we'll add proper CRLF
        writeHeader(pOutputBuffer, STATUS_OK, getContentType(sRealPath), FileStats.st_size);
        evbuffer_add(pOutputBuffer, CRLF, 2);
    } else {
        writeHeader(pOutputBuffer, STATUS_BAD_REQUEST, TYPE_HTML, MASSAGE_LENGTH_BAD_REQUEST);
        evbuffer_add(pOutputBuffer, MASSAGE_BAD_REQUEST, MASSAGE_LENGTH_BAD_REQUEST);
    }
    return;
}

/*void writeBadRequest(evbuffer *pOutputBuffer)
{
    writeHeader(pOutputBuffer, STATUS_BAD_REQUEST, TYPE_HTML, 17);
    evbuffer_add(pOutputBuffer, MASSAGE_BAD_REQUEST, MASSAGE_LENGTH_BAD_REQUEST);
}*/

const char* getContentType(const std::string& sPath)
{
    int nDotPos = sPath.find_last_of('.');
    std::string sFileExtention = sPath.substr(nDotPos+1);
    if(!strcmp(sFileExtention.c_str(), EXTENTION_HTML)) {
        return TYPE_HTML;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_CSS)) {
        return TYPE_CSS;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_JS)) {
        return TYPE_JS;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_JPG)) {
        return TYPE_JPG;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_JPEG)) {
        return TYPE_JPEG;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_PNG)) {
        return TYPE_PNG;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_GIF)) {
        return TYPE_GIF;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_SWF)) {
        return TYPE_SWF;
    }
    return TYPE_UNKNOWN;
}
void writeHeader(evbuffer *pOutputBuffer, const char* sStatus, const char* sType, int nLength)
{
    char  Headers[]=    "HTTP/1.1 %s\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %d\r\n"
                        "Server: %s\r\n"
                        "Connection: %s\r\n"
                        "Date: %s\r\n";
    const time_t timer = time(NULL);
    //std::cout << sStatus << "\n";
    evbuffer_add_printf(pOutputBuffer, Headers, sStatus, sType,
                        nLength, SERVER_NAME, HEADER_CONNECTION, ctime(&timer));
}

bool validatePath(const std::string sPath)
{
    int nLength = sPath.length();
    int nDepth = 0;
    for(int i = 0; i < nLength;)
    {
        if(sPath[i] == '/'){
            if(sPath[i+1] == '.' && sPath[i+2] == '.' && i < nLength - 2) {
                --nDepth;
                i += 3;
            } else {
                ++nDepth;
                ++i;
            }
        } else {
            ++i;
        }
        if (nDepth < 0) return false;
    }
    return true;
}


void urlDecode(char *dst, const char *src)
{
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
                if (a >= 'a')
                        a -= 'a'-'A';
                if (a >= 'A')
                        a -= ('A' - 10);
                else
                        a -= '0';
                if (b >= 'a')
                        b -= 'a'-'A';
                if (b >= 'A')
                        b -= ('A' - 10);
                else
                        b -= '0';
                *dst++ = 16*a+b;
                src+=3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}


/*#include "httpserver.h"
void writeData(struct bufferevent *bev) {
    fprintf(stdout, "writeData\n");
    char *line;
    size_t n;
    char isIndex = 0;
    struct evbuffer *input =  bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    line = evbuffer_readln(input, &n, EVBUFFER_EOL_CRLF);
    char cmd[MAX_OTHER_SIZE] = {'\0'}, protocol[MAX_OTHER_SIZE] = {'\0'}, path[MAX_PATH_SIZE] = {'\0'}, depthCheck[MAX_PATH_SIZE] = {'\0'};
    httpHeader_t httpHeader;
    httpHeader.command = UNKNOWN_C;
    httpHeader.status = OK;
    httpHeader.length = -1;
    if (n != 0) {
        int scaned = sscanf(line, "%s %s %s\n", cmd, path, protocol);
        memcpy(depthCheck, path, MAX_PATH_SIZE);
        if (scaned == 3) {
            if (!strcmp(cmd, "GET")) {
                httpHeader.command = GET;
            }
            else if (!strcmp(cmd, "HEAD")) {
                httpHeader.command = HEAD;
            }
            else {
                httpHeader.command = UNKNOWN_C;
            }
            if (path[0] != '/') {
                httpHeader.status = BAD_REQUEST;
            }
            if (path[strlen(path) - 1] == '/') {
                strcat(path, "index.html");
                isIndex = 1;
            }
            urlDecode(path);
            httpHeader.type = getContentType(path);
            if (getDepth(depthCheck) == -1) {
                httpHeader.status = FORBIDDEN;
            }
        }
        else {
            httpHeader.status = BAD_REQUEST;
        }
    }
    else {
        httpHeader.status = BAD_REQUEST;
    }
    char fullPath[2048] = {'\0'};
    strcpy(fullPath, ROOT_PATH);
    strcat(fullPath, path);
    fprintf(stdout, "test\n");
    fprintf(stdout, "%s\n", fullPath);
    int fd = -1;
    if (httpHeader.status == OK) {
        fd = open(fullPath, O_RDONLY);
        if (fd < 0) {
            if (isIndex == 0)
                httpHeader.status = NOT_FOUND;
            else
                httpHeader.status = FORBIDDEN;
        } else {
            struct stat st;
            httpHeader.length = lseek(fd, 0, SEEK_END);
            if (httpHeader.length == -1 || lseek(fd, 0, SEEK_SET) == -1) {
                httpHeader.status = BAD_REQUEST;
            }
            if (fstat(fd, &st) < 0) {
                perror("fstat");
            }
        }
    }
    addHeader(&httpHeader, output);
    if (fd != -1 && httpHeader.status == OK && httpHeader.command == GET) {
        evbuffer_set_flags(output, EVBUFFER_FLAG_DRAINS_TO_FD);
        if(evbuffer_add_file(output, fd, 0, httpHeader.length) != 0) {
            perror("add file");
        }
    }
    free(line);
    bufferevent_write_buffer(bev, output);
}
contentType_t getContentType(char* path) {
    fprintf(stdout, "getContentType\n");
    contentType_t type;
    char buf[256] = {'\0'};
    char *pch = strrchr(path, '.');
    if (pch != NULL) {
        memcpy(buf, pch + 1, strlen(path) - (pch - path) - 1);
        if (!strcmp(buf, "html"))
            type = HTML;
        else if (!strcmp(buf, "png"))
            type = PNG;
        else if (!strcmp(buf, "jpg"))
            type = JPG;
        else if (!strcmp(buf, "jpeg"))
            type = JPEG;
        else if (!strcmp(buf, "css"))
            type = CSS;
        else if (!strcmp(buf, "js"))
            type = JS;
        else if (!strcmp(buf, "gif"))
            type = GIF;
        else if (!strcmp(buf, "swf"))
            type = SWF;
        else
            type = UNKNOWN_T;
    }
    else {
        type = UNKNOWN_T;
    }
    return type;
}

void urlDecode(char *src)
{
    fprintf(stdout, "urlDecode\n");
    char d[MAX_PATH_SIZE];
    char *pch = strrchr(src, '?');
    if (pch != NULL) {
        src[pch - src] = '\0';
    }
    char *dst = d, *s = src;
    char a, b;
    while (*src) {
        if ((*src == '%') &&
                ((a = src[1]) && (b = src[2])) &&
                (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a'-'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a'-'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
    strcpy(s, d);
}

int getDepth(char *path) {
    fprintf(stdout, "getDepth\n");
    int depth = 0;
    char *ch = strtok(path, "/");
    while(ch != NULL) {
        if(!strcmp(ch, ".."))
            --depth;
        else
            ++depth;
        if(depth < 0)
            return -1;
        ch = strtok(NULL, "/");
    }
    return depth;
}
void addServerHeader(struct evbuffer *buf) {
    fprintf(stdout, "addServerHeader\n");
        char timeStr[64] = {'\0'};
        time_t now = time(NULL);
        strftime(timeStr, 64, "%Y-%m-%d %H:%M:%S", localtime(&now));
        //print date
        evbuffer_add_printf(buf, "Date: %s\r\n", timeStr);
        //print server
        evbuffer_add_printf(buf, "Server: BESTEUSERVER\r\n");
}

void addHeader(httpHeader_t *h, struct evbuffer *buf) {
    fprintf(stdout, "addHeader\n");
    if(h->status == OK && (h->command == GET || h->command == HEAD)) {
        //print response
        evbuffer_add_printf(buf, "HTTP/1.1 200 OK\r\n");
        addServerHeader(buf);
        //print content type
        evbuffer_add_printf(buf, "Content-Type: ");
        switch(h->type) {
            case HTML:
                evbuffer_add_printf(buf, "text/html");
                break;
            case JPEG:
            case JPG:
                evbuffer_add_printf(buf, "image/jpeg");
                break;
            case PNG:
                evbuffer_add_printf(buf, "image/png");
                break;
            case CSS:
                evbuffer_add_printf(buf, "text/css");
                break;
            case JS:
                evbuffer_add_printf(buf, "application/x-javascript");
                break;
            case GIF:
                evbuffer_add_printf(buf, "image/gif");
                break;
            case SWF:
                evbuffer_add_printf(buf, "application/x-shockwave-flash");
                break;
            default:
                evbuffer_add_printf(buf, "text/html");
        }
        evbuffer_add_printf(buf, "\r\n");
        //print content length
        evbuffer_add_printf(buf, "Content-Length: %lu\r\n", h->length);
        //print connection close
        evbuffer_add_printf(buf, "Connection: close\r\n\r\n");
    }
    else if (h->status == NOT_FOUND) {
        evbuffer_add_printf(buf, "HTTP/1.1 404 NOT FOUND\r\n");
        addServerHeader(buf);
        evbuffer_add_printf(buf, "\r\n");
        evbuffer_add_printf(buf, "404 NOT FOUND");
    } else if (h->status == BAD_REQUEST || h->command == UNKNOWN_C) {
        evbuffer_add_printf(buf, "HTTP/1.1 405 BAD REQUEST\r\n");
        addServerHeader(buf);
        evbuffer_add_printf(buf, "\r\n");
        evbuffer_add_printf(buf, "405 BAD REQUEST");
    } else if (h->status == FORBIDDEN) {
        evbuffer_add_printf(buf, "HTTP/1.1 403 FORBIDDEN\r\n");
        addServerHeader(buf);
        evbuffer_add_printf(buf, "\r\n");
        evbuffer_add_printf(buf, "FORBIDDEN");
    }
}


*/
