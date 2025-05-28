#include "../../include/request/Get.hpp"
#include "../../include/config/Location.hpp" // Include the header file for Location

Get::Get()
{
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

std::string Get::ListingDir(const std::string &path, std::string request_path)
{
    //To Check the auto indexing Option  Conf File

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
    /*
    std::cout << "GET RELATIVE PATH !!!!!!!!!!!!!!  "<< cur_location->get_return().size() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
    std::cout << "GET RELATIVE PATH !!!!!!!!!!!!!!  "<< cur_location->get_path() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
    std::cout << "GET RELATIVE PATH !!!!!!!!!!!!!!  "<< cur_location->get_root_location() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
    std::cout << "GET RELATIVE PATH !!!!!!!!!!!!!!  "<< cur_location->get_alias() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
    */
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
        // const std::string &method = cur_location->get_allowMethods()[i];
        if (i > 0)
            std::cout << ", ";
        // Print the method
        std::cout <<  cur_location->get_allowMethods()[i];
    }
    for (size_t i = 0; i < cur_location->get_return().size(); i++)
    {
        if (i > 0)
            std::cout << ", ";
        std::cout << "return Redirection  : " << cur_location->get_return()[i];
    }
    if (!cur_location->get_return().empty())
    {        
        this->setIsRedirected(true);
        std::cout << "REDIRECTED TO : " << cur_location->get_return()[0] << std::endl;
        std::cout << "REQUEST LOCATION : " << request->GetLocation()  << std::endl;
        // exit(0);
        rel_path = cur_location->get_return()[1];
        return rel_path;
    }
    else if (!cur_location->get_alias().empty())
    {
        rel_path = cur_location->get_alias() + request->GetLocation();
        std::cout << "ALIAS PATH : " << rel_path << std::endl;
        // if (rel_path[rel_path.size() - 1] != '/')
        //     rel_path += '/';
        std::cout << "RELATIVE PATH : " << rel_path << std::endl;
        return rel_path;
    }
    else if (!cur_location->get_root_location().empty())
    {
        rel_path = cur_location->get_root_location() + request->GetLocation();
        std::cout << "ROOT LOCATION PATH : " << rel_path << std::endl;
        // if (rel_path[rel_path.size() - 1] != '/')
        //     rel_path += '/';
        std::cout << "RELATIVE PATH : " << rel_path << std::endl;
        return rel_path;
    }
    // If no alias or root location is specified, use the Server's root path
    rel_path = request->GetClientDatat()->_server->getServerConfig().get_root() + request->GetLocation();
    return rel_path;
}

// @Todo : before making any edit to this function, check the work flow of the processRequest function ~
void    Get::ProccessRequest(HttpRequest *request)
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
    if(cur_location == NULL)
    {
        std::cerr << "NOT FOUND LOCATION \n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    rel_path = GetRelativePath(cur_location, request);
    std::cout << "RELATIVE PATH : " << rel_path << ":::::::::::::" << std::endl;
    
    if (rel_path.empty())
    {
        std::cerr << "NOT FOUND RELATIVE PATH \n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    // Check if the relative path is valid
    if (!IsValidPath(rel_path))
    {
        std::cerr << "NOT FOUND INVALID LOCATION \n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    if (IsDir(rel_path))
    {
        request->GetClientDatat()->http_response->setStatusCode(200);
        request->GetClientDatat()->http_response->setStatusMessage("OK");
        request->GetClientDatat()->http_response->setContentType("text/html");
        request->GetClientDatat()->http_response->setChunked(false);
        
        std::string indexFile = rel_path + "index.html";
        std::cout << "INDEX FILE : " << indexFile << std::endl;
        if (IsFile(indexFile))
        {
            std::cout << "INDEX FILE IS VALID-- " << rel_path + "index.html\n";
            request->GetClientDatat()->http_response->setFilePath(rel_path + "index.html");
            return;
        }
        else
        {
            std::cout << "INDEX FILE IS NOT VALID-- " << indexFile << "\n";
            std::string response = ListingDir(rel_path, request->GetLocation());
            
            if (response.empty())
            {
                response = "<html><body><h1>Directory Listing Not Available</h1></body></html>";
            }
            
            std::cout << "RESPONSE :>>>>>>>>>>>>>>>>>> " << response << std::endl;
            request->GetClientDatat()->http_response->setBuffer(response);
            return;
        }
    }
    else
    {
        std::cout << "FILE IS VALID-- " << rel_path << "\n";
        request->GetClientDatat()->http_response->setFilePath(rel_path);
        return;
    }
}
