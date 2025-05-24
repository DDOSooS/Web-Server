#include "../../include/request/Get.hpp"

Get::Get()
{
}

Get::~Get()
{

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

std::string Get::ListingDir(const std::string &path)
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
    
    path_ = "www" + normalized_path;  // Use a relative path that exists in your project structure
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
        if (href_path[href_path.length()-1] != '/')
            href_path += "/";
            
        response << "<li> <a href=\"" << href_path << rs_name;
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

void    Get::ProccessRequest(HttpRequest *request)
{
    if (!request) {
        std::cerr << "Error: Null request pointer\n";
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }

    if (!request->GetClientDatat()) {
        std::cerr << "Error: Null client data pointer\n";
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }

    if (!request->GetClientDatat()->http_response) {
        std::cerr << "Error: Null http_response pointer\n";
        std::map<std::string, std::string> emptyHeaders;
        request->GetClientDatat()->http_response = new HttpResponse(200, emptyHeaders, "text/html", false, false);
    }

    // Normalize the location path
    std::string location = request->GetLocation();
    if (location.empty()) {
        location = "/";
    } else if (location[0] != '/') {
        location = "/" + location;
    }

    std::string root_path = "www";  // Use a relative path that exists in your project structure
    std::string rel_path = root_path + location;
    
    std::cout << "RELATIVE PATH : " << rel_path << std::endl;
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
        
        // Make sure directory path ends with a slash
        if (rel_path[rel_path.length() - 1] != '/') {
            rel_path += "/";
        }
        
        // Make sure location ends with a slash for proper URL construction
        if (location[location.length() - 1] != '/') {
            location += "/";
        }
        
        std::string indexFile = rel_path + "index.html";
        std::cout << "INDEX FILE : " << indexFile << std::endl;
        
        if (IsFile(indexFile))
        {
            std::cout << "INDEX FILE IS VALID-- " << location + "index.html\n";
            request->GetClientDatat()->http_response->setFilePath(location + "index.html");
            return;
        }
        else
        {
            std::cout << "INDEX FILE IS NOT VALID-- " << indexFile << "\n";
            std::string response = ListingDir(location);
            
            if (response.empty()) {
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
        request->GetClientDatat()->http_response->setFilePath(location);
        return;
    }
}
