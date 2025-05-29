#include "../../include/request/Get.hpp"
#include "../../include/config/Location.hpp" // Include the header file for Location

Get::Get()
{
    this->_is_redirected = false;
}

Get::~Get()
{

}

void Get::setIsRedirected(bool is_redirected)
{
    this->_is_redirected = is_redirected;
}

bool Get::getIsRedirected() const
{
    return this->_is_redirected;
}

bool Get::CanHandle(std::string method)
{
    return method == "GET";
}


bool    Get::IsValidPath(const std::string &path)
{
    struct stat _statinfo;
    std::cout << "PATH : (((" << path << ")))" << std::endl;

    return (stat(path.c_str(), &_statinfo) == 0);
}

bool    Get::IsDir(const std::string &path)
{
    struct stat _statinfo;

    if (stat(path.c_str(), &_statinfo) != 0)
        return (false);
    return (S_ISDIR(_statinfo.st_mode));
}

bool    Get::IsFile(const std::string &path)
{
    struct stat _statinfo;
    
    if (stat(path.c_str(), &_statinfo) != 0)
        return false;
    return (S_ISREG(_statinfo.st_mode));
}

bool Get::check_auto_indexing(const Location *cur_location, const ServerConfig &serverConfig)
{
    if (!cur_location)
    {
        if (serverConfig.get_autoindex())
            return true;
        std::cerr << "[ WARNING ] : No matching location found, auto indexing cannot be checked." << std::endl;
        return false;
    }
    return cur_location->get_autoindex();
}

std::string Get::ListingDir(const std::string &path, std::string request_path, const Location *cur_location,const ServerConfig &serverConfig)
{
    //To Check the auto indexing Option  Conf File

    if (!check_auto_indexing(cur_location, serverConfig))
    {
        std::cerr << "[ WARNING ] : Auto indexing is disabled for this location." << std::endl;
        return "";
    }
    DIR *dir;
    struct dirent *entry;
    std::stringstream response;
    std::string path_;
    
    // Make sure the path starts with a slash
    std::string normalized_path = path;
    if (normalized_path.empty() || normalized_path[0] != '/')
        normalized_path = "/" + normalized_path;
    
    path_ = normalized_path;  // Use a relative path that exists in your project structure
    std::cout << "Opening directory: " << path_ << std::endl;
    
    dir = opendir(path_.c_str());
    if (dir == NULL) {
        std::cerr << "Failed to open directory: " << path_ << " - " << strerror(errno) << std::endl;
        return "";
    }
    
    response << "<html>\n"
    << "<head>\n<title>Index of " << path << "</title>\n"
    << "<style>\n"
    << "    body { font-family: Arial, sans-serif; margin: 20px; }\n"
    << "    h1 { border-bottom: 1px solid #ccc; padding-bottom: 10px; }\n"
    << "    ul { list-style-type: none; padding: 0; }\n"
    << "    li { margin: 5px 0; }\n"
    << "    li a { text-decoration: none; }\n"
    << "    li a:hover { text-decoration: underline; }\n"
    << "    .folder { font-weight: bold; }\n"
    << "</style>\n"
    << "</head>\n"
    << "<body>\n"
    << "<h1>Index of " << path << "</h1>\n"
    << "<ul>\n";
    
    // Add parent directory link if not at root
    if (path != "/") {
        response << "<li><a href=\"";
        size_t lastSlash = path.rfind('/');
        if (lastSlash != std::string::npos) {
            response << path.substr(0, lastSlash);
        } else {
            response << "/";
        }
        response << "\" class=\"folder\">..</a></li>\n";
    }
    
    while ((entry = readdir(dir)) != NULL)
    {
        std::string rs_name;//ressource name
  
        rs_name = entry->d_name;
        if (rs_name == "." || rs_name == "..")
            continue;
        
        // Make sure path ends with a slash
        std::string href_path = path;
        if (request_path[request_path.length()-1] != '/')
            request_path += "/";
        std::cout << "[ DEBUG ] : href_path : " << href_path << std::endl;
        std::cout << "[ DEBUG ] : request_path : " << request_path << std::endl;
        std::cout << "[ DEBUG ] : rs_name : " << rs_name << std::endl;
        response << "<li> <a href=\"" << request_path << rs_name;
        if (entry->d_type == DT_DIR)
           response << "/\" class=\"folder\">"  << rs_name << "/";
        else if (entry->d_type == DT_REG)
            response << "\" class=\"file\">" << rs_name;
        response << "</a></li>\n";
    }
    response << "</ul>\n";
    response << "</body>\n";
    response << "</html>\n";
    closedir(dir);
    return response.str();
}

/*
    case scenario for an http request 
    incomming request !!!!
    | |
    -_-
    check if the location exist in the conf file -> (NO) check the / location ->(NO) -> 404 error page
                                                |
                                                 -> (Yes) handle location as it is 
    
*/
/*
    Nginx processes directives in this order of precedence:
    nginxlocation /example {
        # 1. RETURN - Highest priority (stops processing)
        return 301 /new-location;
        
        # 2. ALIAS - If no return, and alias exists
        alias /var/www/files/;
        
        # 3. ROOT - If no return and no alias
        root /var/www/html;
        
        # 4. Other directives (try_files, etc.)
        try_files $uri $uri/ =404;
    }
*/

std::string Get::determineContentType(const std::string& path)
{
    int size;
    size_t dot_pos;
    
    std::string contentType[] = { "html", "css", "js", "png", "jpeg", "gif", "json", "xml" , "pdf", "mp4", "mpeg", "x-www-form-urlencoded", "form-data", "woff2", "woff", "zip", "csv"};
    std::string contenetFormat[] = { "text/html", "text/css", "application/javascript", "image/png", "image/jpeg", "image/gif", "application/json", "application/xml", "application/pdf", "video/mp4", "audio/mpeg", "application/x-www-form-urlencoded", "multipart/form-data", "font/woff2", "font/woff", "application/zip", "text/csv"};
    size = sizeof(contentType) / sizeof(contentType[0]);
    dot_pos = path.rfind('.');
    if (dot_pos != std::string::npos)
    {
        std::string ext = path.substr(dot_pos + 1);
        for (int i = 0; i < size; i++)
            if (contentType[i] == ext)
        return contenetFormat[i];
    }
    return "text/plain";
}

std::string    Get::GetRelativePath(const Location * cur_location,HttpRequest *request)
{
    std::string rel_path;

    // in case of no location found, 
    if (!cur_location)
    {
        rel_path = request->GetClientDatat()->_server->getServerConfig().get_root() + request->GetLocation();
        if (rel_path[rel_path.length() - 1] != '/')
            rel_path += '/';
        std::cout << "[ WARNING ] : No matching location found, using server root: " << rel_path << std::endl;
        return rel_path;
    }

    // in case the location is just a default
    /*
    std::cout << "[ DEBUG ] : LOCATION INFOT\n";
    std::cout << "PATH : " << cur_location->get_path() << std::endl;
    std::cout << "ROOT LOCATION : " << cur_location->get_root_location() << std::endl;
    std::cout << "ALIAS : " << cur_location->get_alias() << std::endl;
    std::cout << "RETURN : " << cur_location->get_return().size() << std::endl;
    std::cout << "AUTO INDEX : " << (cur_location->get_autoindex() ? "ON" : "OFF") << std::endl;
    std::cout << "INDEX : " << cur_location->get_index() << std::endl;
    std::cout << "ALLOW METHODS : ";
    for (size_t i = 0; i < cur_location->get_allowMethods().size(); i++)
    {
        if (i > 0)
        std::cout << ", ";
        std::cout <<  cur_location->get_allowMethods()[i];
    }
    for (size_t i = 0; i < cur_location->get_return().size(); i++)
    {
        if (i > 0)
        std::cout << ", ";
        std::cout << "return Redirection  : " << cur_location->get_return()[i];
    }
    std::cout << std::endl;
    std::cout << "CLIENT MAX BODY SIZE : " << cur_location->get_clientMaxBodySize() << std::endl;
    std::cout << "[====================================================]\n";
    */
    if (!cur_location->get_return().empty())
    {        
        this->setIsRedirected(true);
        std::cout << "REDIRECTED TO : " << cur_location->get_return()[0] << std::endl;
        std::cout << "REDIRECTED TO : " << cur_location->get_return()[1] << std::endl;
        std::cout << "REQUEST LOCATION : " << request->GetLocation()  << std::endl;
        rel_path = cur_location->get_return()[1];
        return rel_path;
    }
    else if (!cur_location->get_alias().empty())
    {
        std::cout << "[ DEBUG ] : Using alias : " << cur_location->get_alias() << std::endl;
        rel_path = cur_location->get_alias() + request->GetLocation();
    }
    else if (!cur_location->get_root_location().empty())
    {
        std::cout << "[ DEBUG ] : Using root location : " << cur_location->get_root_location() << std::endl;
        rel_path = cur_location->get_root_location() + request->GetLocation();
    }
    else if (rel_path.empty())
    {
        std::cerr << "[ WARNING ] : No alias or root location specified, using server root." << std::endl;
        // If no alias or root location is specified, we use the Server's root path
        std::cout << "[ Server Root Path :" << request->GetClientDatat()->_server->getServerConfig().get_root() << " ]\n";
        rel_path = request->GetClientDatat()->_server->getServerConfig().get_root() + request->GetLocation();
    }
    if (rel_path[rel_path.length() - 1] != '/')
    {
        rel_path += '/';
    }
    else
    {
        std::cout << "[ DEBUG ] : Relative Path already ends with a slash : !!!!!!!!!!!" << rel_path << std::endl;
    }
    std::cout << "[------------ FInal rel_path :" << rel_path << " ----------------------]\n";
    return rel_path;
}

// @Todo : before making any edit to this function, check the work flow of the processRequest function ~
void    Get::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig)
{
    std::cout << "PROCCESSING GET REQUEST !!!!!!!!!\n";

    std::string     rel_path;
    const Location *cur_location;
    
    if (!request)
    {
        std::cerr << "Error: Null request pointer\n";
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    if (!request->GetClientDatat())
    {
        std::cerr << "Error: Null client data pointer\n";
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    cur_location = request->GetClientDatat()->_server->getServerConfig().findMatchingLocation(request->GetLocation());
    rel_path = GetRelativePath(cur_location, request);
    std::cout << "\n[---------[DEBUG] : Relative Path : " << rel_path << "--------------------]" << std::endl;
    if (this->getIsRedirected())
    {
        std::stringstream ss;
        int               status_code;

        ss << cur_location->get_return()[0];
        ss >> status_code;
        std::cout << "[REDIRECTED TO : " << rel_path  << " ]"<< std::endl;
        request->GetClientDatat()->http_response->setStatusCode(status_code);
        request->GetClientDatat()->http_response->setStatusMessage("Moved Permanently");
        request->GetClientDatat()->http_response->setBuffer(" ");
        request->GetClientDatat()->http_response->setChunked(false);
        request->GetClientDatat()->http_response->setHeader("Location", rel_path);
        return;
    }
    if (rel_path.empty())
    {
        std::cerr << "[empty rel_path Not Found ]\n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    // Check if the relative path is valid
    if (!IsValidPath(rel_path))
    {
        std::cerr << "[ Debug ] : file is not a Valid one \n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    else
        std::cout << "[Debug] : Valid Path Found : " << rel_path << std::endl;
    if (IsDir(rel_path))
    {
        request->GetClientDatat()->http_response->setStatusCode(200);
        request->GetClientDatat()->http_response->setStatusMessage("OK");
        request->GetClientDatat()->http_response->setContentType("text/html");
        request->GetClientDatat()->http_response->setChunked(false);
        std::string indexFile = rel_path + request->GetClientDatat()->_server->getServerConfig().get_index();
        std::cout << "INDEX FILE : " << indexFile << std::endl;
        if (IsFile(indexFile))
        {
            request->GetClientDatat()->http_response->setFilePath(rel_path + "index.html");
            return;
        }
        else
        {
            std::string response = ListingDir(rel_path, request->GetLocation(), cur_location, serverConfig);
            if (response.empty())
                   throw HttpException(403, "403 Forbidden", FORBIDDEN);
            request->GetClientDatat()->http_response->setBuffer(response);
            return;
        }
    }
    else
    {
        request->GetClientDatat()->http_response->setFilePath(rel_path);
        return;
    }
}
