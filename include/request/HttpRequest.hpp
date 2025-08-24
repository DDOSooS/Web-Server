#pragma once
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include "../config/Location.hpp"
#include "../config/ServerConfig.hpp"

class ClientConnection;
enum RequestStatus
{
    PARSER,
    PROCESSING,
    RESPONDING,
    DONE,
    ERROR
};

enum RequestLineStatus
{
    REQ_PROCESSING,
    REQ_HTTP_VERSION_ERROR,
    REQ_METHOD_ERROR,
    REQ_NOT_IMPLEMENTED,
    REQ_LOCATION_ERROR,
    REQ_DONE
};

enum UploadingStatus
{
        UPLOAD_START,
        UPLOAD_BOUNDARY_SEARCH,
        UPLOAD_FILENAME_SEARCH,
        UPLOAD_PROCESSING,
        UPLOAD_DONE,
        UPLOAD_ERROR
};


class HttpRequest
{
    private:
        ClientConnection *                                  _client;
        int                                                 _socket_fd;
        std::string                                         _request_line;
        std::string                                         _http_version;
        std::string                                         _method;
        std::string                                         _location;
        std::string                                         _buffer;
        std::vector<char>                                   _body_vec;
        std::string                                         _body;
        std::map<std::string, std::string>                  _headers;
        std::vector<std::pair<std::string, std::string> >   _query_string;
        std::string                                         _query_string_str;
        enum RequestStatus                                  _status;
        bool                                                _is_crlf;
        RequestLineStatus                                   _is_rl; //request line
        bool                                                _are_header_parsed;
        bool                                                _is_redirected; // Flag to indicate if the request is redirected or not
        bool                                                _processed; // Flag to indicate if the request has been processed
        size_t                                              _remaine_bytes;
        size_t                                              _recv_bytes;
        size_t                                              _body_start;
        size_t                                              _body_size;
        std::string                                         _upload_file_path;
        bool                                                _is_streaming_upload;
        int                                                 _upload_file_type; 
        std::string                                         _boundry;
        enum UploadingStatus                               _uploading_status;

    public:
        HttpRequest();
        HttpRequest(HttpRequest const &);
        bool                                            FindHeader(std::string, std::string);
        std::string                                     GetHeader(std::string )const;
        bool                                            HasHeader(std::string key) const;
        std::map<std::string, std::string>              GetHeaders()const;
        std::string                                     GetRequestLine() const;
        std::string                                     GetHttpVersion() const;
        std::vector<char>                               GetBodyAsBin() const;
        std::string                                     GetBodyAsStr() const;
        std::vector<std::pair<std::string, std::string> > GetQueryString() const;
        bool                                            GetIsCrlf() const;
        RequestLineStatus                               GetIsRl() const;
        std::string                                     GetMethod() const;
        std::string                                     GetLocation() const;
        ClientConnection *                              GetClientDatat() const;
        enum RequestStatus                              GetStatus() const;
        std::string                                     GetQueryStringStr() const;
        bool                                            IsRedirected() const;
        bool                                            IsProcessed() const;
        std::string                                     GetRelativePath(const Location * cur_location, ClientConnection *client);  ;
        int                                             GetRedirectCounter() const;
        size_t                                          GetRemaineBytes() const;
        size_t                                          GetRecvBytes() const;
        size_t                                          GetBodyStart() const;
        size_t                                          GetBodySize() const;
        int                                             GetSocketFd() const;
        std::string                                     GetUploadFilePath() const;
        std::string                                     GetBoundary() const;
        enum UploadingStatus                            GetUploadingStatus() const;


        void                                           SetUploadStatus(enum UploadingStatus status);
        void                                           SetBoundary(std::string boundry);
        void                                            SetSocketFd(int);
        void                                            SetUploadFilePath(std::string);
        void                                            SetIsStreamingUpload(bool);
        void                                            SetRedirectCounter(int);
        void                                            SetRemaineBytes(size_t);
        void                                            SetRecvBytes(size_t);
        void                                            SetBodyStart(size_t);
        void                                            SetBodySize(size_t);
        void                                            SetIsRedirected(bool);
        void                                            SetProcessed(bool);
        void                                            SetQueryStringStr(std::string);
        void                                            SetClientData(ClientConnection *);
        void                                            SetMethod(std::string);
        void                                            SetRequestLine(std::string);
        void                                            SetHttpVersion(std::string);
        void                                            SetLocation(std::string);
        void                                            SetHeader(std::string, std::string);
        void                                            SetBodyAsBin(std::vector<char> );
        void                                            SetBodyAsStr(std::string );
        void                                            SetBuffer(std::string);
        void                                            SetIsCrlf(bool);
        void                                            SetIsRl(RequestLineStatus);
        void                                            SetAreHeaderParsed(bool);
        void                                            SetStatus(enum RequestStatus );
        void                                            SetQueryString(std::vector<std::pair<std::string, std::string> > );
        void                                            ResetRequest();
        std::string                                     GetRelativePath(const Location * cur_location);
        bool                                            IsValidRequest() const;
        void                                            handleRedirect(const Location * cur_location , std::string &rel_path);
        std::string                                     GetRedirectionMessage(int status_code) const;
        bool                                            IsStreamingUpload() const;
        int                                             GetUploadFileType() const;
        ~HttpRequest();
};

size_t                                          GetFileSize(std::string &file) ; 