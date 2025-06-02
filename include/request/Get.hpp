#pragma once

#include "./RequestHandler.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

// Remove this line - you don't need it since you're including RequestHandler.hpp
// class RequestHandler;

class Get : public RequestHandler
{
    private:
        bool _is_redirected; // Flag to indicate if the request is redirected or not

    public:
        Get();
        bool            CanHandle(std::string method);
        void            ProccessRequest(HttpRequest *, const ServerConfig &serverConfig);
        std::string     IsValidPath( std::string &path);
        bool            IsDir( std::string &path);
        bool            IsFile( std::string &path);
        std::string     ListingDir(const std::string &path, std::string /* request Path*/, const Location * /* location*/,const ServerConfig & /*config file*/);
        std::string     determineContentType(const std::string& path);
        bool            check_auto_indexing(const Location * /* location*/, const ServerConfig & /*config file*/);
        std::string     CheckIndexFile(const std::string &rel_path, const Location *cur_location, const ServerConfig &serverConfig);
        ~Get();
};