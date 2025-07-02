#pragma once

#include "./RequestHandler.hpp"

class Post : public RequestHandler
{
    public:
        Post();
        ~Post();

        bool CanHandle(std::string method) override;
        void ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig) override;
};
