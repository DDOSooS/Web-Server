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
    /*
        handle uploading data first we need to check the content type and the content length
        if it's content length is more than 1mb we should handle it as a streaming upload
        if it's content type is multipart/form-data we should handle it as a multipart upload
          -- check for the boundary in the content type header and file name in the content disposition header
          -- start reading the boshoulddy and parse it accordingly
        if it's text/plain we  handle it as a plain text upload
          -- read the body and save it to a tmp file generate a respionse with 200 status code and the unlink the tmp file
        if it's url encode we could store the data in a container and generate a response with 200 status code
        //json upload is not supported yet  
    */
    //empty body handling
    std::cout << request->GetBody().size() << std::endl;
    if (request->GetBody().empty())
    {
        throw HttpException(400, "Bad Request - Body should be empty for POST requests", BAD_REQUEST);
    }
    // check if the content length exceeds the maximum allowed size To do @@@
    // getting the file name and the file extension
    // if (request->GetHeader("Content-Type").find("multipart/form-data") != std::string::npos)
    // {
    //     handleMultipartFormData(request, serverConfig, clientConfig);
    // }
     if (request->GetHeader("Content-Type") == "application/x-www-form-urlencoded")
    {
        std::cout << "Handling application/x-www-form-urlencoded data" << std::endl;
        handleUrlEncodedData(request, serverConfig, clientConfig);
    }
    else if (request->GetHeader("Content-Type") == "text/plain")
    {
        std::cout << "Handling text/plain data" << std::endl;
        hanleTextPlainData(request, serverConfig, clientConfig);
    }
    // else
    // {
    //     std::cout << "Unsupported Content-Type: " << request->GetHeader("Content-Type") << std::endl;
    //     throw HttpException(415, "Unsupported Media Type", BAD_REQUEST);
    // }
    // std::cout << request->GetHeader("Content-Type") << std::endl;
    // exit(0);
}

void Post::hanleTextPlainData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    std::string body = request->GetBody() + "   Data received successfully";
    std::cout << "Received text/plain data: " << body << std::endl;

    // Save to a temporary file or process as needed
    // For now, just generate a response with 200 status code
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setBuffer(body);
    request->GetClientDatat()->http_response->setContentType("text/plain");
    request->GetClientDatat()->_server->updatePollEvents(request->GetSocketFd() ,POLLOUT);
}

std::string UrlDecode(const std::string &req_line)
{
    std::string decoded;

    for (size_t i = 0; i < req_line.size(); i++)
    {
        if (req_line[i] == '%' && i + 2 < req_line.size())
        {
            int hexChar;
            if (sscanf(req_line.substr(i+1,3).c_str(), "%x", &hexChar) == 1)
            {
                decoded += static_cast<char>(hexChar);
                i += 2;
            }
            else
                decoded += req_line[i];
        }
        else if (req_line[i] == '+')
            decoded += ' ';
        else
            decoded += req_line[i];
    }
    return decoded;
}
void Post::handleUrlEncodedData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    std::string body = request->GetBody();
    // std::cout << "Received application/x-www-form-urlencoded data: " << body << std::endl;

    // Process the URL-encoded data
    std::unordered_map<std::string, std::string> formData;
    for (size_t i = 0; i < body.size(); ++i)
    {
        size_t pos = body.find_first_of('&', i);
        if (pos == std::string::npos)
        {
            pos = body.find('=', i);
            if (pos != std::string::npos)
            {
                formData[body.substr(i, pos - i)] = UrlDecode (body.substr(pos + 1));
            }
            else
            {
                formData[body.substr(i)] = "";
            }
            break; 
        }
        std::string input_field = body.substr(i, pos - i);
        std::cout << input_field << std::endl;
        // exit(1);
        size_t tmp_pos = input_field.find('=');
        if (tmp_pos != std::string::npos)
            formData[input_field.substr(0, tmp_pos)] = UrlDecode(input_field.substr(tmp_pos + 1, pos - tmp_pos - 1));
        else
            formData[input_field] = "";
        
        i = pos; 
    }
    std::cout << "===============" << std::endl;
    for (const auto& pair : formData)
    {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
    // std::cout << "Body content: " << body << std::endl;
    std::stringstream ss(body);

    ss << "<html><body>";
    ss << "<h1>Form Data Received</h1>";
    ss << "<table border='1'><tr><th>Key</th><th>Value</th></tr>";
    for (const auto& pair : formData)
    {
        ss << "<tr><td>" << pair.first << "</td><td>" << pair.second << "</td></tr>";
    }
    ss << "</table>";
    ss << "</body></html>";
    body = ss.str();
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setBuffer(body);
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->_server->updatePollEvents(request->GetSocketFd() ,POLLOUT);
}

