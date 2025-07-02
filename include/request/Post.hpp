#pragma once

#include "./RequestHandler.hpp"

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
};
