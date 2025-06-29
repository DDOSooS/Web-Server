#pragma once
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <istream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include "../request/HttpException.hpp"

#define CHUNKED_SIZE 1024
class HttpResponse
{
    private:
        int                                                 _status_code;
        std::map<std::string, std::string>                  _headers;
        std::string                                         _status_message;
        std::string                                         _content_type;

        // if the response is a string
        std::string                                         _buffer;

        // if the response is a file
        std::string                                         _file_path;


        bool                                                _is_chunked;
        bool                                                _keep_alive;
        int                                                 _byte_sent;
        int                                                 _byte_to_send;

    public:
        HttpResponse(int , std::map<std::string, std::string>, std::string, bool, bool);

        void                                                setStatusCode(int code);
        void                                                setStatusMessage(std::string message);
        void                                                setHeader(std::string key, std::string value);
        void                                                setFilePath(std::string path);
        void                                                setChunked(bool chunked);
        void                                                setKeepAlive(bool keep_alive);
        void                                                setBuffer(std::string buffer);
        void                                                setContentType(std::string content_type);
        void                                                setByteSent(int byte_sent);
        void                                                setByteToSend(int byte_to_send);


        int                                              getByteToSend() const;
        int                                                 getStatusCode() const;
        std::string                                         getStatusMessage() const;
        std::string                                         getHeader(std::string key) const;
        std::string                                         getFilePath() const;
        bool                                                isChunked() const;
        bool                                                isKeepAlive() const;
        std::string                                         getBuffer() const;
        int                                              getByteSent() const;
        std::string                                         getContentType() const;
        std::map<std::string, std::string>                  getHeaders() const;

        bool                                                checkAvailablePacket() const;

        void                                                sendChunkedResponse(int socekt_fd);
        void                                                sendResponse(int socket_fd);
        std::string  toString() ;
        std::string  GetStatusMessage(int code) const;
        void         clear();
        bool         isFile() const;
        std::string determineContentType( std::string path) ;
        ~HttpResponse();
};

