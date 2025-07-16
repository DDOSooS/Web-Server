#pragma once

#include "./RequestHandler.hpp"

#define MAX_CHUNK_SIZE 1024 * 1024 * 5 ;

class Post : public RequestHandler
{
    public:
        Post();
        ~Post();

        bool CanHandle(std::string method) override;
        void ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig) override;
        void handleMultipartFormData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig);
        void handleUrlEncodedData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig);
        void hanleTextPlainData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig);
        void writeReamingbody(HttpRequest *request, std::string uploadPath);
        bool checkBoundary(std::string body, std::string &boundary) const;
        void    writeToTargetFile(HttpRequest* request,std::string & upload_path, std::string & bounedary);
        void validateLocation(const Location *location);
        void readBodyByChunks(HttpRequest *request);
};
