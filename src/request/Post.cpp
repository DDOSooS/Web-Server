#include "../../include/request/Post.hpp"

Post::Post()
{

}

Post::~Post()
{

}

bool Post::CanHandle(std::string method)
{
    return method == "POST";
}


/*
    i should check if there is any body left fromt the parsing part
    i should check the content length to deteriminate if i am gonna read at once or ishould read in chunks or 
    it 's a chunked transfer encoding
    i should check if the content type is multipart/form-data or not
    i should check if the content type is application/json or not
    i should check if the content type is application/x-www-form-urlencoded or not
    i should check if the content type is text/plain or not
*/

void    Post::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    // CHECKING IF THE CONTENT LENGTH EXCEEDS THE MAXIMUM ALLOWED SIZE
    // size_t max_size = clientConfig.get_client_max_body_size()
    // to check does it conserne the server or just a specific location TO DO
    std::cout << "Processing POST request" << std::endl;
    std::cout << "---- [REQUEST DETAILS] ----" << std::endl;
    std::cout << "Method: " << request->GetMethod() << std::endl ;
    std::cout << "Location: " << request->GetLocation() << std::endl;
    std::cout << "Headers: " << std::endl;
    for (const auto& header : request->GetHeaders())
    {
        std::cout << "  key [" << header.first << " ] : Value [ " << header.second << " ]" << std::endl;
    }
    std::cout << "Body: " << request->GetBody() << std::endl;
    std::cout << "NUMBER OF HEADERS: " << request->GetHeaders().size() << std::endl;
    std::cout << "---- [END OF REQUEST DETAILS] ----" << std::endl;
    // START UPLOADING DATA
    
    exit(0);
}
