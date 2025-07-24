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
        bool checkBoundary(std::string body, std::string boundary) const;
        void writeToTargetFile(HttpRequest* request, std::string uploadPath);
        void handleFirstCall(HttpRequest *request, std::ofstream &ofs, std::string bounedary);
        void validateLocation(const Location *location);
        void readBodyChunk(HttpRequest *request);
        void getBodyReamiingBytes(HttpRequest *request);
        void uploading_logger(HttpRequest *request);
        bool isEndBoundary(const std::string& data, const std::string& boundary) ;
        size_t findEndBoundaryPosition(const std::string& data, const std::string& boundary) ;
        static int counter;
};
