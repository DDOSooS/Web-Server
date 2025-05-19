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
    
    path_ =  "/home/aghergho/Desktop/Web-Server/www" + path;
    dir = opendir(path_.c_str());
    if (dir == NULL)
        return ("");
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
    while ((entry = readdir(dir)) != NULL)
    {
        std::string rs_name;//ressource name
  
        rs_name = entry->d_name;
        if ( rs_name == "." || rs_name == "..")
            continue;
        response << "<li> <a href=\"" << path ;
        if (entry->d_type == DT_DIR)
           response << "/" << rs_name  << "\" class=\"folder\">"  << rs_name << "/";
        else if (entry->d_type == DT_REG)
            response << "/" << rs_name << "\" class=\"file\">" << rs_name;
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
    std::string rel_path;

    rel_path = "/home/aghergho/Desktop/Web-Server/www" + request->GetLocation();
    std::cout << "RELATIVE PATH : " << rel_path << std::endl;
    if (!IsValidPath(rel_path))
    {
        std::cerr << "NOT FOUND INVALID LOCATION \n";
        throw HttpException(404, "404 Not Found", ERROR_TYPE::NOT_FOUND);
    }
    if (IsDir(rel_path))
    {
        request->GetClientDatat()->http_response->setStatusCode(200);
        request->GetClientDatat()->http_response->setStatusMessage("OK");
        request->GetClientDatat()->http_response->setContentType("text/html");
        request->GetClientDatat()->http_response->setChunked(false);
        std::string indexFile;
        
        indexFile = rel_path + "index.html";
        std::cout << "INDEX FILE : " << indexFile << std::endl;
        if (IsFile(indexFile))
        {
            std::cout << "INDEX FILE IS VALID-- " << request->GetLocation() + "index.html\n";
            request->GetClientDatat()->http_response->setFilePath(request->GetLocation() + "index.html");
            return ;
        }
        else
        {
            std::cout << "INDEX FILE IS NOT VALID-- " <<  indexFile<<"\n";
            std::string response;

            response = ListingDir(request->GetLocation());
            std::cout << "RESPONSE :>>>>>>>>>>>>>>>>>> " << response << std::endl;
            request->GetClientDatat()->http_response->setBuffer(response);
            return ;
        }
    }
    else
    {
        std::cout << "FILE IS VALID-- " << rel_path << "\n";
        request->GetClientDatat()->http_response->setFilePath(request->GetLocation());
        return ;
    }
}
