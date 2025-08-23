#include "../../include/request/Post.hpp"
#include <unordered_map>
#include <algorithm>
#include <string.h>

#define MAX_CHUNK_SIZE 1024 * 1024 * 7 

int Post::counter = 0;

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

// CHECKING IF THE CONTENT LENGTH EXCEEDS THE MAXIMUM ALLOWED SIZE
// size_t max_size = clientConfig.get_client_max_body_size()
// to check does it conserne the server or just a specific location TO DO
// std::cout << "Processing POST request" << std::endl;
// std::cout << "---- [REQUEST DETAILS] ----" << std::endl;
// std::cout << "Method: " << request->GetMethod() << std::endl ;
// std::cout << "Location: " << request->GetLocation() << std::endl;
// std::cout << "Headers: " << std::endl;
// for (const auto& header : request->GetHeaders())
// {
//     std::cout << "  key [" << header.first << " ] : Value [ " << header.second << " ]" << std::endl;
// }
// std::cout << "Body: " << request->GetBodyAsStr() << std::endl;
// std::cout << "NUMBER OF HEADERS: " << request->GetHeaders().size() << std::endl;
// std::cout << "---- [END OF REQUEST DETAILS] ----" << std::endl;

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

/*
    checking if it's the location 

*/
void    Post::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{

    std::cout << "--- checking for post location ---" << std::endl;

    if (request->GetBodyAsStr().empty() && !request->IsStreamingUpload())
    {
        throw HttpException(400, "Bad Request - Body should be empty for POST requests", BAD_REQUEST);
    }
    if (!request->HasHeader("Content-Type") && request->HasHeader("transfer-encoding") && request->GetHeader("transfer-encoding") == "chunked")
    {
        throw HttpException(400, "Bad Request - Content-Type header is missing", BAD_REQUEST);
    }

    if (request->GetHeader("Content-Type").find("multipart/form-data") != std::string::npos)
    {
        handleMultipartFormData(request, serverConfig, clientConfig);
    }
    else if (request->GetHeader("Content-Type") == "application/x-www-form-urlencoded")
    {
        handleUrlEncodedData(request, serverConfig, clientConfig);
    }
    else if (request->GetHeader("Content-Type") == "text/plain")
    {
        hanleTextPlainData(request, serverConfig, clientConfig);
    }
    else
    {
        std::cout << "Unsupported Content-Type: " << request->GetHeader("Content-Type") << std::endl;
        throw HttpException(415, "Unsupported Media Type", BAD_REQUEST);
    }
}

bool fileExists(const std::string& filename)
{
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

bool isDirectory(const std::string& path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0)
    {
        return S_ISDIR(buffer.st_mode);
    }
    return false;
}

bool createDirectoryIfNotExists(const std::string& path)
{
    if (isDirectory(path))
        return true;
    
    // Try to create directory
    if (mkdir(path.c_str(), 0755) == 0)
    {
        std::cout << "Created directory: " << path << std::endl;
        return true;
    }
    
    std::cerr << "Failed to create directory: " << path << " - " << std::endl;
    return false;
}

// Save to a temporary file or process as needed
// For now, just generate a response with 200 status code
void Post::hanleTextPlainData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    (void)clientConfig; 
    (void)serverConfig;
    std::string received = request->GetBodyAsStr();
    std::stringstream ss;
    ss << "<!DOCTYPE html>\n";
    ss << "<html lang=\"en\">\n";
    ss << "<head>\n";
    ss << "    <meta charset=\"UTF-8\">\n";
    ss << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    ss << "    <title>Text Data Received - Postman Style</title>\n";
    ss << "    <style>\n";
    ss << "        body { background: #23272f; color: #fff; font-family: 'Segoe UI', 'Inter', Arial, sans-serif; margin: 0; padding: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center; }\n";
    ss << "        .card { background: #2a2e37; border-radius: 16px; box-shadow: 0 8px 32px rgba(255,107,53,0.15); padding: 2.5rem 2rem; max-width: 500px; width: 100%; text-align: center; border: 1px solid #ff6b35; }\n";
    ss << "        h1 { color: #ff6b35; font-size: 2rem; margin-bottom: 1.2rem; }\n";
    ss << "        .data-box { background: #23272f; border: 1px solid #444; border-radius: 8px; padding: 1rem; margin-bottom: 1.5rem; color: #ff8c42; font-size: 1.1rem; word-break: break-all; }\n";
    ss << "        .btn { background: linear-gradient(90deg, #ff6b35, #ff8c42); color: #fff; border: none; border-radius: 8px; padding: 0.8rem 2rem; font-size: 1rem; font-weight: 600; cursor: pointer; transition: background 0.2s; text-decoration: none; display: inline-block; margin-top: 1.5rem; }\n";
    ss << "        .btn:hover { background: linear-gradient(90deg, #ff8c42, #ff6b35); }\n";
    ss << "        .postman-logo { margin-top: 2rem; opacity: 0.7; }\n";
    ss << "    </style>\n";
    ss << "</head>\n";
    ss << "<body>\n";
    ss << "    <div class=\"card\">\n";
    ss << "        <h1>Text Data Received</h1>\n";
    ss << "        <div class=\"data-box\">" << received << "</div>\n";
    ss << "        <div style=\"color:#b8b8b8; margin-bottom:1.5rem;\">Data received successfully.</div>\n";
    ss << "        <a href=\"/\" class=\"btn\">Back to Home</a>\n";
    ss << "        <div class=\"postman-logo\">\n";
    ss << "            <svg width=\"80\" height=\"32\" viewBox=\"0 0 80 32\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    ss << "                <circle cx=\"16\" cy=\"16\" r=\"16\" fill=\"#ff6b35\"/>\n";
    ss << "                <rect x=\"36\" y=\"12\" width=\"40\" height=\"8\" rx=\"4\" fill=\"#ff6b35\"/>\n";
    ss << "            </svg>\n";
    ss << "        </div>\n";
    ss << "    </div>\n";
    ss << "</body>\n";
    ss << "</html>\n";
    std::string body = ss.str();
    std::cout << "Received text/plain data: " << received << std::endl;

    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setBuffer(body);
    request->GetClientDatat()->http_response->setContentType("text/html");
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

std::string generateUniqueFileName(const std::string &base_path, const std::string &file_name)
{
    std::string unique_file_name = file_name;
    std::stringstream ss(unique_file_name);
    // std::cout << "Base path: " << filn << std::endl;
    std::stringstream ts_ss;
    ts_ss << std::time(NULL);
    std::string timestamp = ts_ss.str();
	if (base_path[base_path.length() - 1] != '/')
		ss << base_path << "/";
	else
		ss << base_path;
    ss << timestamp << "_" << file_name;
    return ss.str();
}

void Post::handleUrlEncodedData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    (void)serverConfig; 
    (void)clientConfig; 
    std::string body = request->GetBodyAsStr();

    std::cout << "Received application/x-www-form-urlencoded data: " << body << std::endl;
    std::unordered_map<std::string, std::string> formData;
    for (size_t i = 0; i < body.size(); ++i)
    {
        size_t pos = body.find_first_of('&', i);
        if (pos == std::string::npos)
        {
            pos = body.find('=', i);
            if (pos != std::string::npos)
                formData[body.substr(i, pos - i)] = UrlDecode (body.substr(pos + 1));
            else
                formData[body.substr(i)] = "";
            break; 
        }
        std::string input_field = body.substr(i, pos - i);
        std::cout << input_field << std::endl;
        size_t tmp_pos = input_field.find('=');
        if (tmp_pos != std::string::npos)
            formData[input_field.substr(0, tmp_pos)] = UrlDecode(input_field.substr(tmp_pos + 1, pos - tmp_pos - 1));
        else
            formData[input_field] = "";
        
        i = pos; 
    }
    std::stringstream ss;
    ss << "<!DOCTYPE html>\n";
    ss << "<html lang=\"en\">\n";
    ss << "<head>\n";
    ss << "    <meta charset=\"UTF-8\">\n";
    ss << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    ss << "    <title>Form Data Received - Postman Style</title>\n";
    ss << "    <style>\n";
    ss << "        body { background: #23272f; color: #fff; font-family: 'Segoe UI', 'Inter', Arial, sans-serif; margin: 0; padding: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center; }\n";
    ss << "        .card { background: #2a2e37; border-radius: 16px; box-shadow: 0 8px 32px rgba(255,107,53,0.15); padding: 2.5rem 2rem; max-width: 500px; width: 100%; text-align: center; border: 1px solid #ff6b35; }\n";
    ss << "        h1 { color: #ff6b35; font-size: 2rem; margin-bottom: 1.2rem; }\n";
    ss << "        .table-wrap { overflow-x: auto; margin-bottom: 1.5rem; }\n";
    ss << "        table { width: 100%; border-collapse: collapse; background: #23272f; margin: 0 auto; }\n";
    ss << "        th, td { padding: 0.7rem 1rem; border-bottom: 1px solid #444; text-align: left; }\n";
    ss << "        th { background: #ff6b35; color: #fff; font-weight: 600; }\n";
    ss << "        tr:last-child td { border-bottom: none; }\n";
    ss << "        tr:nth-child(even) td { background: #262a33; }\n";
    ss << "        .btn { background: linear-gradient(90deg, #ff6b35, #ff8c42); color: #fff; border: none; border-radius: 8px; padding: 0.8rem 2rem; font-size: 1rem; font-weight: 600; cursor: pointer; transition: background 0.2s; text-decoration: none; display: inline-block; margin-top: 1.5rem; }\n";
    ss << "        .btn:hover { background: linear-gradient(90deg, #ff8c42, #ff6b35); }\n";
    ss << "        .postman-logo { margin-top: 2rem; opacity: 0.7; }\n";
    ss << "    </style>\n";
    ss << "</head>\n";
    ss << "<body>\n";
    ss << "    <div class=\"card\">\n";
    ss << "        <h1>Form Data Received</h1>\n";
    ss << "        <div class=\"table-wrap\">\n";
    ss << "        <table>\n";
    ss << "            <tr><th>Key</th><th>Value</th></tr>\n";
    for (std::unordered_map<std::string, std::string>::iterator it = formData.begin(); it != formData.end(); ++it)
    {
        ss << "            <tr><td>" << it->first << "</td><td>" << it->second << "</td></tr>\n";
    }
    ss << "        </table>\n";
    ss << "        </div>\n";
    ss << "        <a href=\"/\" class=\"btn\">Back to Home</a>\n";
    ss << "        <div class=\"postman-logo\">\n";
    ss << "            <svg width=\"80\" height=\"32\" viewBox=\"0 0 80 32\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    ss << "                <circle cx=\"16\" cy=\"16\" r=\"16\" fill=\"#ff6b35\"/>\n";
    ss << "                <rect x=\"36\" y=\"12\" width=\"40\" height=\"8\" rx=\"4\" fill=\"#ff6b35\"/>\n";
    ss << "            </svg>\n";
    ss << "        </div>\n";
    ss << "    </div>\n";
    ss << "</body>\n";
    ss << "</html>\n";
    body = ss.str();
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setBuffer(body);
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->_server->updatePollEvents(request->GetSocketFd() ,POLLOUT);
}


std::string IsValidPath( std::string path) 
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

                std::cout << "IT'S A VALID PATH" << std::endl;
                exit(1);
                return path;
            }
        }
    }
    std::cout << "[ INFO ] : Path is a file: " << path << std::endl;
    return path;
}

std::string extractFileName(const std::string &body)
{
    size_t start;
    size_t end;
    
    start = body.find("filename=\"");
    end = body.find("\"", start + 10);
    if (start == std::string::npos || end == std::string::npos)
    {
        return "";
    }
    return ( body.substr(start + 10, end - start - 10));
}

bool Post::checkBoundary(std::string body, std::string boundary) const
{
    size_t pos = body.find(boundary);
    if (pos == std::string::npos)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void Post::validateLocation(const Location *location)
{
	if (!location)
		throw HttpException(404, "Not Found - Location not found", NOT_FOUND);
	if (location)
	{
		std::vector<std::string> allowed_methods = location->get_allowMethods();
		if (std::find(allowed_methods.begin(), allowed_methods.end(), "POST") == allowed_methods.end())
			throw HttpException(405, "Method Not Allowed - ", METHOD_NOT_ALLOWED);
		if (location->get_uploadStore().empty())
			throw HttpException(500, "Internal Server Error - Upload store not configured", INTERNAL_SERVER_ERROR);
		std::string uploadStore = location->get_uploadStore();
		if (!isDirectory(uploadStore))
		{
			throw HttpException(500, "Internal Server Error  - Upload-store Directory doesn't exist ", INTERNAL_SERVER_ERROR);
		}
	}
}


void Post::readBodyChunk(HttpRequest *request)
{
    std::cout << "Reading body chunk for multipart/form-data" << std::endl;
    size_t buffer_size = std::min(request->GetRemaineBytes(), static_cast<size_t>(MAX_CHUNK_SIZE));
    
    char buffer[buffer_size];
    size_t bytesRead = 0;
    bytesRead = recv(request->GetSocketFd(), buffer, buffer_size, 0);
    if (!bytesRead)
    {
        throw HttpException(500, "Internal Server Error - Failed to read body chunk", INTERNAL_SERVER_ERROR);
    }
    else if (bytesRead == 0)
    {
        return;
    }
    request->SetBodyAsStr(request->GetBodyAsStr() + std::string(buffer, bytesRead));
}



void Post::getBodyReamiingBytes(HttpRequest *request)
{
    size_t pos;

    pos = request->GetBodyAsStr().find("\r\n\r\n");
    if (pos == std::string::npos)
    {
        return;
    }
    else
    {
        std::fstream file(request->GetUploadFilePath(), std::ios::out | std::ios::binary | std::ios::app);
        if (!file.is_open())
        {
            request->SetIsStreamingUpload(false);
            throw HttpException(500, "Internal Server Error - Failed to open file for writing", INTERNAL_SERVER_ERROR);
        }
        file.write(request->GetBodyAsStr().substr(pos + 4).c_str(), request->GetBodyAsStr().size() - pos - 4);
        if (file.fail())
        {
            std::cout << "Failed to write remaining bytes to file: " << request->GetUploadFilePath() << std::endl;
        }
        file.close();
        request->SetRemaineBytes( request->GetBodySize() - request->GetBodyAsStr().size());
        request->SetRecvBytes (request->GetBodyAsStr().size());
        request->SetBodyAsStr("");
    }
    uploading_logger(request);
}
void Post::uploading_logger(HttpRequest *request)
{
    std::cout << "Uploading prgogress: " << (request->GetRecvBytes() * 100 / request->GetBodySize()) << "%" << std::endl;
}

std::string DecodeHtmlEntities(const std::string& encoded)
{
    std::string decoded;
    size_t i = 0;
    while (i < encoded.length())
    {
        if (encoded[i] == '&' && i + 2 < encoded.length() && encoded[i+1] == '#' )
        {
            size_t semi = encoded.find(';', i+2);
            if (semi != std::string::npos)
            {
                std::string numstr = encoded.substr(i+2, semi-(i+2));
                bool is_number = true;
                for (size_t j = 0; j < numstr.length(); ++j)
                {
                    if (numstr[j] < '0' || numstr[j] > '9')
                    {
                        is_number = false;
                        break;
                    }
                }
                if (is_number && !numstr.empty())
                {
                    int codepoint = 0;
                    for (size_t j = 0; j < numstr.length(); ++j)
                        codepoint = codepoint * 10 + (numstr[j] - '0');
                    std::string utf8_char;
                    if (codepoint <= 0x7F) // 0 - 127
                        utf8_char = static_cast<char>(codepoint);
                    else if (codepoint <= 0x7FF) // 128 - 2047
                    {
                        utf8_char += static_cast<char>(0xC0 | (codepoint >> 6));
                        utf8_char += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else if (codepoint <= 0xFFFF) // 2048 - 65535
                    {
                        utf8_char += static_cast<char>(0xE0 | (codepoint >> 12));
                        utf8_char += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        utf8_char += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else // 65536 - 1114111
                    {
                        utf8_char += static_cast<char>(0xF0 | (codepoint >> 18));
                        utf8_char += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                        utf8_char += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        utf8_char += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    decoded += utf8_char;
                    i = semi + 1;
                    continue;
                }
            }
        }
        decoded += encoded[i];
        ++i;
    }
    return decoded;
}

void Post::writeToTargetFile(HttpRequest *request, std::string upload_file)
{
    std::fstream file(upload_file, std::ios::out | std::ios::binary | std::ios::app);
    if (!file.is_open())
    {
        request->SetIsStreamingUpload(false);
        throw HttpException(500, "Internal Server Error - Failed to open file for writing", INTERNAL_SERVER_ERROR);
    }
    
    size_t buffer_size = std::min(request->GetRemaineBytes(), static_cast<size_t>(MAX_CHUNK_SIZE));
    char buffer[buffer_size ]; 
    size_t byte_read = recv(request->GetSocketFd(), buffer, buffer_size, 0);
    
    if (!byte_read)
    {
        request->SetIsStreamingUpload(false);
        throw HttpException(500, "Internal Server Error - Failed to read body chunk", INTERNAL_SERVER_ERROR);
    }
    else if (byte_read == 0)
    {
        request->SetIsStreamingUpload(false);
        uploading_logger(request);
        file.close();
        return;
    }
    
    std::string chunk_data(buffer, byte_read);
    std::string closing_boundary = "\r\n" + request->GetBoundary() + "--";
    size_t boundary_pos = chunk_data.find(closing_boundary);
    if (boundary_pos != std::string::npos)
    {
        file.write(buffer, boundary_pos);
        // std::cout << "Upload completed - found closing boundary at position: " << boundary_pos << std::endl;
        request->SetIsStreamingUpload(false);
        std::cout << "File upload completed!" << std::endl;
    }
    else 
        file.write(buffer, byte_read);
    request->SetRemaineBytes(request->GetRemaineBytes() - byte_read);
    request->SetRecvBytes(request->GetRecvBytes() + byte_read);

    file.close();
}
 
/*
    1 -is it the first time to handle multipart form data
        check if boundary exist and if it's not wait until the next call ;
    2- if the boundary is being a valid one 
        = > i should read the body until the reach remaining bytes to 0 
*/
// validate Location and also upload store 
/*
    we do have 2 scenarios here
    -> the body contains the boundary and filename ;
    -> finding the boundary doesn't mean that we have the filename
    -> if it's not we shoul wait until we do find the boundary and filename => then read the body until we reach the end of the body ( remaining bytes = 0 )
    */
/*
    at this point we are sure that we have a valid boundary and a file name so we can proceed to read the body from the end of the first crlf
    we should read the body until we reach the end of the body ( remaining bytes = 0 )
    first we should process the reamingbody part that exist in request body before starting to read the body in chunks    
*/

void Post::handleMultipartFormData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    (void)clientConfig;
    this->counter++;
    const Location *location = serverConfig.findMatchingLocation(request->GetLocation());
    if (!request->IsStreamingUpload())
    {
        validateLocation(location);
        if (request->HasHeader("Content-Type")  && request->GetBodySize() > location->get_clientMaxBodySize())
            throw HttpException(400, "Payload Too Large", BAD_REQUEST);
        if (request->HasHeader("Content-Type") && request->GetBodySize() == 0)
            throw HttpException(400, "Bad Request - No file data provided", BAD_REQUEST);
        std::string bounedary;
        bounedary = request->GetHeader("Content-Type");
        if (bounedary.find("boundary=") != std::string::npos)
        {
			bounedary = bounedary.substr(bounedary.find("boundary=") + 9);
            request->SetBoundary("--" + bounedary);
        }
        else
        {
			request->SetIsStreamingUpload(false);
            throw HttpException(400, "Bad Request - Boundary not found", BAD_REQUEST);
        }
        request->SetUploadStatus(UPLOAD_BOUNDARY_SEARCH);
    }
    //handling empty body
    if (request->GetUploadingStatus() == UPLOAD_BOUNDARY_SEARCH)
    {
        if (!request->IsStreamingUpload() && request->GetBodyAsStr().empty())
        {
            readBodyChunk(request);
            request->SetIsStreamingUpload(true);
            return;
        }
        else if (request->IsStreamingUpload() && request->GetBodyAsStr().empty() && !request->GetRecvBytes())
        {
            request->SetIsStreamingUpload(false);
            throw HttpException(400, "Bad Request - No data to process", BAD_REQUEST);
            return;
        }
        // hanling the case where we have the boundary and the body is not empty
        if (request->GetBodyAsStr().size() > request->GetBoundary().size())
        {
            if (!checkBoundary(request->GetBodyAsStr(), request->GetBoundary()))
            {
                request->SetIsStreamingUpload(false);
                throw HttpException(400, "Bad Request - Boundary not found", BAD_REQUEST);
            }
            else
            {
                request->SetUploadStatus(UPLOAD_FILENAME_SEARCH);
            }
        }
        else
        {
            readBodyChunk(request);
            return;
        }
    }
    // we should check for the filename in the body
    if (request->GetUploadingStatus() == UPLOAD_FILENAME_SEARCH)
    {
        if (request->GetBodyAsStr().find("filename=\"") != std::string::npos)
        {
            std::string file_name = extractFileName(request->GetBodyAsStr());
            if (file_name.empty())
            {
                request->SetIsStreamingUpload(false);
                throw HttpException(400, "Bad Request - No file name found", BAD_REQUEST);
            }
            else
            {
                file_name = DecodeHtmlEntities(file_name);
                std::string uploadStore = location->get_uploadStore();
                std::string unique_file_name = generateUniqueFileName(uploadStore, file_name);
                request->SetUploadFilePath(unique_file_name);
                request->SetUploadStatus(UPLOAD_PROCESSING);
            }
        }
        else
        {
            request->SetIsStreamingUpload(false);
            throw HttpException(400, "Bad Request - No file name found", BAD_REQUEST);
        }
    }
    // checking if the body contains any remaining bytes
    if (request->GetUploadingStatus() == UPLOAD_PROCESSING)
    {
        if (!request->GetBodyAsStr().empty())
        {
            getBodyReamiingBytes(request);
            if (request->GetRemaineBytes() > 0)
            {
                request->SetIsStreamingUpload(true);
                return ;
            }
        }
        if (request->GetRemaineBytes() > 0)
            writeToTargetFile(request, request->GetUploadFilePath());
        if (request->GetRemaineBytes() == 0)
        {
            request->SetUploadStatus(UPLOAD_DONE);
            request->SetIsStreamingUpload(false);
            request->GetClientDatat()->http_response->setStatusCode(200);
            request->GetClientDatat()->http_response->setStatusMessage("OK");
            std::string html = "<!DOCTYPE html>\n"
                "<html lang=\"en\">\n"
                "<head>\n"
                "    <meta charset=\"UTF-8\">\n"
                "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                "    <title>Upload Complete - Postman Style</title>\n"
                "    <style>\n"
                "        body { background: #23272f; color: #fff; font-family: 'Segoe UI', 'Inter', Arial, sans-serif; margin: 0; padding: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center; }\n"
                "        .card { background: #2a2e37; border-radius: 16px; box-shadow: 0 8px 32px rgba(255,107,53,0.15); padding: 2.5rem 2rem; max-width: 400px; width: 100%; text-align: center; border: 1px solid #ff6b35; }\n"
                "        .checkmark { font-size: 3.5rem; color: #ff6b35; margin-bottom: 1rem; }\n"
                "        h1 { color: #ff6b35; font-size: 2rem; margin-bottom: 0.5rem; }\n"
                "        p { color: #b8b8b8; font-size: 1.1rem; margin-bottom: 1.5rem; }\n"
                "        .btn { background: linear-gradient(90deg, #ff6b35, #ff8c42); color: #fff; border: none; border-radius: 8px; padding: 0.8rem 2rem; font-size: 1rem; font-weight: 600; cursor: pointer; transition: background 0.2s; text-decoration: none; display: inline-block; }\n"
                "        .btn:hover { background: linear-gradient(90deg, #ff8c42, #ff6b35); }\n"
                "        .postman-logo { margin-top: 2rem; opacity: 0.7; }\n"
                "    </style>\n"
                "</head>\n"
                "<body>\n"
                "    <div class=\"card\">\n"
                "        <div class=\"checkmark\">&#10003;</div>\n"
                "        <h1>Upload Complete!</h1>\n"
                "        <p>Your file has been uploaded successfully.</p>\n"
                "        <a href=\"/\" class=\"btn\">Back to Home</a>\n"
                "        <div class=\"postman-logo\">\n"
                "            <svg width=\"80\" height=\"32\" viewBox=\"0 0 80 32\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\">\n"
                "                <circle cx=\"16\" cy=\"16\" r=\"16\" fill=\"#ff6b35\"/>\n"
                "                <rect x=\"36\" y=\"12\" width=\"40\" height=\"8\" rx=\"4\" fill=\"#ff6b35\"/>\n"
                "            </svg>\n"
                "        </div>\n"
                "    </div>\n"
                "</body>\n"
                "</html>\n";
            request->GetClientDatat()->http_response->setBuffer(html);
            request->GetClientDatat()->http_response->setContentType("text/html");
            std::cout << "File uploaded successfully" << std::endl;
        }
        else if (request->GetRemaineBytes() > 0 )
        {
            std::cout << "Waiting for more data to process multipart/form-data!!!3" << std::endl;
            request->SetIsStreamingUpload(true);
            return ;
        }
    }
}
