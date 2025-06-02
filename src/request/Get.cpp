#include "../../include/request/Get.hpp"
#include "../../include/config/Location.hpp" 

Get::Get()
{
    this->_is_redirected = false;
}

Get::~Get()
{

}

bool Get::CanHandle(std::string method)
{
    return method == "GET";
}


std::string Get::IsValidPath( std::string &path)
{
    struct stat _statinfo;
    
    if (stat(path.c_str(), &_statinfo) != 0)
    {
        if (path[path.length() - 1] == '/')
        {
            path = path.substr(0, path.length() - 1);
            if (stat(path.c_str(), &_statinfo) != 0)
            {
                std::cerr << "[ ERROR ] : Path does not exist: " << path << std::endl;
                return "";
            }
            else
            {
                std::cout << "[ INFO ] : Path is a directory: " << path << std::endl;
                return path;
            }
        }
    }
    std::cout << "[ INFO ] : Path is a file: " << path << std::endl;
    return path;
}

bool    Get::IsDir( std::string &path)
{
    struct stat _statinfo;

    if (stat(path.c_str(), &_statinfo) != 0)
        return (false);
    return (S_ISDIR(_statinfo.st_mode));
}

bool    Get::IsFile( std::string &path)
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
    std::string normalized_path = path;
    
    if (normalized_path.empty() || normalized_path[0] != '/')
        normalized_path = "/" + normalized_path;
    path_ = normalized_path;  // Use a relative path that exists in your project structure
    // std::cout << "Opening directory: " << path_ << std::endl;
    dir = opendir(path_.c_str());
    if (dir == NULL)
    {
        std::cerr << "Failed to open directory: " << path_ << " - " << strerror(errno) << std::endl;
        return "";
    }
    response << "<html>\n"
    << "<head>\n<title>Index of " << request_path << "</title>\n"
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
    << "<h1>Index of " << request_path << "</h1>\n"
    << "<ul>\n";
    
    // Add parent directory link if not at root
    if (path != "/") {
        response << "<li><a href=\"";
        size_t lastSlash = path.rfind('/');
        if (lastSlash != std::string::npos)
            response << path.substr(0, lastSlash);
        else
            response << "/";
        response << "\" class=\"folder\">..</a></li>\n";
    }    
    while ((entry = readdir(dir)) != NULL)
    {
        std::string rs_name;//ressource name
  
        rs_name = entry->d_name;
        if (rs_name == "." || rs_name == "..")
            continue;
        // Making !!! sure path ends with a slash
        std::string href_path = path;
        if (request_path[request_path.length()-1] != '/')
            request_path += "/";
        /*
            std::cout << "[ DEBUG ] : request_path : " << request_path << std::endl;
            std::cout << "[ DEBUG ] : rs_name : " << rs_name << std::endl;
            std::cout << "[ DEBUG ] : href_path : " << href_path << std::endl;
        */
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

std::string Get::CheckIndexFile(const std::string &rel_path, const Location *cur_location, const ServerConfig &serverConfig)
{
    std::string indexFile;
    
    if (!cur_location)
    {
        std::cout << "[ DEBUG ] : No matching location found, using server index file: " << indexFile << std::endl;
        // this was updtated to use the server's index file as a vector of strings
        if (serverConfig.get_index().empty())
        {
            std::cerr << "[ ERROR ] : No index file specified in server configuration." << std::endl;
            return "";
        }
        // Check if the server's index file exists
        for (size_t i = 0; i < serverConfig.get_index().size(); i++)
        {
            std::cout << "[ DEBUG ] : Checking index file: " << serverConfig.get_index()[i] << std::endl;
            indexFile = rel_path + serverConfig.get_index()[i];
            if (IsFile(indexFile))
            {
                std::cout << "[ DEBUG ] : Index file found at: " << indexFile << std::endl;
                return indexFile;
            }
        }
    }
    else
    {
        // i'm supposing that the index file are in vector of strings Done a ssi abdeslame mrhba si ayoube
        //std::cout << "[ DEBUG ] : Checking index file for current location: " << cur_location->get_index() << std::endl;
        for (size_t i = 0; i < cur_location->get_index().size(); i++)
        {
            indexFile = rel_path + cur_location->get_index()[i];
            std::cout << "[ DEBUG ] : Checking index file: " << indexFile << std::endl;
            if (IsFile(indexFile))
            {
                std::cout << "[ DEBUG ] : Index file found at: " << indexFile << std::endl;
                return indexFile;
            }
        }
    }
    return "";
}
// @Todo : before making any edit to this function, check the work flow of the processRequest function ~
void    Get::ProccessRequest(HttpRequest *request,const ServerConfig &serverConfig)
{
    std::string rel_path;
    const Location *cur_location;
    std::cout << "[DEBUG ]: --- PROCCESSING GET REQUEST ---\n";
    
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
    // Get the current location from the server configuration
    cur_location = serverConfig.findBestMatchingLocation(request->GetLocation());
    rel_path = request->GetRelativePath(cur_location);
    if (rel_path.empty())
    {
        std::cerr << "[empty rel_path Not Found ]\n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    // Check if the relative path is valid
    rel_path =  IsValidPath(rel_path);
    if (rel_path.empty())
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
        /*
        // std::cout << "[Debug] : Index file check for current location : " << cur_location->get_index() << std::endl;
        std::cout << "[Debug] : Checking error page for the server configuration :" << std::endl;
        std::map<short, std::string> error_page = serverConfig.get_error_pages();
        for (std::map<short, std::string>::iterator i =error_page.begin(); i != error_page.end(); i++)
        {
            std::cout << "Error Code: " << i->first << ", Page: " << i->second << std::endl;
        }
        */
        /* check if there is any valid index file from the list of index files !!!*/
        std::string indexFile = CheckIndexFile(rel_path, cur_location, serverConfig);
        if (!indexFile.empty())
        {
            std::cout << "[Debug] : Index file found : " << indexFile << std::endl;
            request->GetClientDatat()->http_response->setFilePath(indexFile);
            request->GetClientDatat()->http_response->setContentType(determineContentType(indexFile));
            request->GetClientDatat()->http_response->setBuffer("");
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
