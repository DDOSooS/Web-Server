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
        void            ProccessRequest(HttpRequest *);
        bool            IsValidPath(const std::string &path);
        bool            IsDir(const std::string &path);
        bool            IsFile(const std::string &path);
        std::string     ListingDir(const std::string &path, std::string /* request Path*/);
        std::string     determineContentType(const std::string& path);
        bool            getIsRedirected() const;
        void            setIsRedirected(bool );
        std::string     GetRelativePath(const Location * cur_location,HttpRequest *request);
        ~Get();
};