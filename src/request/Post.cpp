#include "../../include/request/Post.hpp"
#include <unordered_map>

int Post::counter = 0;
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
    std::cout << "Body: " << request->GetBodyAsStr() << std::endl;
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


    /*
        checking if it's the location 
    
    */

    std::cout << "--- checking for post location ---" << std::endl;
    const Location *location = serverConfig.findMatchingLocation(request->GetLocation());
    std::cout << "Location found: " << (location ? location->get_path() : "Not Found") << std::endl;
    std::cout << "upload store: " << (location ? location->get_uploadStore() : "Not Found") << std::endl;
    // exit(0);
    //empty body handling
    std::cout << request->GetBodyAsStr().size() << std::endl;
    if (request->GetBodyAsStr().empty() && !request->IsStreamingUpload())
    {
        throw HttpException(400, "Bad Request - Body should be empty for POST requests", BAD_REQUEST);
    }
    if (!request->HasHeader("Content-Type") && request->HasHeader("transfer-encoding") && request->GetHeader("transfer-encoding") == "chunked")
    {
        throw HttpException(400, "Bad Request - Content-Type header is missing", BAD_REQUEST);
    }

    // check if the content length exceeds the maximum allowed size To do @@@
    // getting the file name and the file extension
    if (request->GetHeader("Content-Type").find("multipart/form-data") != std::string::npos)
    {
        handleMultipartFormData(request, serverConfig, clientConfig);
    }
    else if (request->GetHeader("Content-Type") == "application/x-www-form-urlencoded")
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
    
    std::cerr << "Failed to create directory: " << path << " - " << strerror(errno) << std::endl;
    return false;
}

void Post::hanleTextPlainData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    std::string body = request->GetBodyAsStr() + "   Data received successfully";
    std::cout << "Received text/plain data: " << body << std::endl;

    // Save to a temporary file or process as needed
    // For now, just generate a response with 200 status code
    request->GetClientDatat()->http_response->setStatusCode(200);
    request->GetClientDatat()->http_response->setStatusMessage("OK");
    request->GetClientDatat()->http_response->setBuffer(body);
    request->GetClientDatat()->http_response->setContentType("text/plain");
    request->GetClientDatat()->_server->updatePollEvents(request->GetSocketFd() ,POLLOUT);
}

/*
// Text files
    {"txt", "text/plain"},
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"json", "application/json"},
    {"xml", "application/xml"},
    {"csv", "text/csv"},
    {"md", "text/markdown"},
    {"rtf", "application/rtf"},
    
    // Programming languages
    {"c", "text/x-c"},
    {"cpp", "text/x-c++"},
    {"cc", "text/x-c++"},
    {"cxx", "text/x-c++"},
    {"h", "text/x-c"},
    {"hpp", "text/x-c++"},
    {"java", "text/x-java"},
    {"py", "text/x-python"},
    {"php", "text/x-php"},
    {"rb", "text/x-ruby"},
    {"go", "text/x-go"},
    {"rs", "text/x-rust"},
    {"sh", "text/x-shellscript"},
    {"bat", "text/x-msdos-batch"},
    {"ps1", "text/x-powershell"},
    {"sql", "text/x-sql"},
    
    // Images
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"gif", "image/gif"},
    {"bmp", "image/bmp"},
    {"svg", "image/svg+xml"},
    {"webp", "image/webp"},
    {"ico", "image/x-icon"},
    {"tiff", "image/tiff"},
    {"tif", "image/tiff"},
    
    // Audio
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"flac", "audio/flac"},
    {"aac", "audio/aac"},
    {"ogg", "audio/ogg"},
    {"m4a", "audio/mp4"},
    
    // Video
    {"mp4", "video/mp4"},
    {"avi", "video/x-msvideo"},
    {"mov", "video/quicktime"},
    {"wmv", "video/x-ms-wmv"},
    {"flv", "video/x-flv"},
    {"webm", "video/webm"},
    {"mkv", "video/x-matroska"},
    
    // Documents
    {"pdf", "application/pdf"},
    {"doc", "application/msword"},
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"xls", "application/vnd.ms-excel"},
    {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {"odt", "application/vnd.oasis.opendocument.text"},
    {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {"odp", "application/vnd.oasis.opendocument.presentation"},
    
    // Archives
    {"zip", "application/zip"},
    {"rar", "application/x-rar-compressed"},
    {"7z", "application/x-7z-compressed"},
    {"tar", "application/x-tar"},
    {"gz", "application/gzip"},
    {"bz2", "application/x-bzip2"},
    
    // Executables
    {"exe", "application/x-msdownload"},
    {"msi", "application/x-msdownload"},
    {"deb", "application/x-debian-package"},
    {"rpm", "application/x-rpm"},
    {"dmg", "application/x-apple-diskimage"},
    
    // Fonts
    {"ttf", "font/ttf"},
    {"otf", "font/otf"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"eot", "application/vnd.ms-fontobject"}
};

// Common text file extensions
const std::unordered_set<std::string> FileHelper::textExtensions = {
    "txt", "html", "htm", "css", "js", "json", "xml", "csv", "md", "rtf",
    "c", "cpp", "cc", "cxx", "h", "hpp", "java", "py", "php", "rb", "go", 
    "rs", "sh", "bat", "ps1", "sql", "yaml", "yml", "ini", "cfg", "conf",
    "log", "gitignore", "gitattributes", "dockerfile", "makefile", "cmake",
    "toml", "properties", "env", "editorconfig", "eslintrc", "prettierrc"
};*/
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
    size_t counter = 1;
    std::stringstream ss(unique_file_name);
    // std::cout << "Base path: " << filn << std::endl;
    std::string timestamp = std::to_string(std::time(nullptr));
	if (base_path.back() != '/')
		ss << base_path << "/";
	else
		ss << base_path;
    ss << timestamp << "_" << file_name;
    return ss.str();
}

void Post::handleUrlEncodedData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    std::string body = request->GetBodyAsStr();

    std::cout << "Received application/x-www-form-urlencoded data: " << body << std::endl;
    // exit(0);
    // Process the URL-encoded data
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

std::string extractFileName(const std::string &body, const std::string &boundary)
{
    size_t start = body.find("filename=\"");
    if (start == std::string::npos)
        return "";

    start += 10; // Length of "filename=\""
    size_t end = body.find("\"", start);
    if (end == std::string::npos)
        return "";

    return body.substr(start, end - start);
}

void    Post::writeReamingbody(HttpRequest *request, std::string uploadPath)
{
    // if ()
    size_t pos = request->GetBodyAsStr().find("/r/n/r/n");

}

bool Post::checkBoundary(std::string body, std::string boundary) const
{
    size_t pos = body.find(boundary);
    if (pos == std::string::npos)
    {
        std::cerr << "Boundary not found in body" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Boundary found: " << boundary << std::endl;
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
			// std::cout << "Upload store path is not a directory or doesn't exist: " << uploadStore << std::endl;
			throw HttpException(500, "Internal Server Error  - Upload-store Directory doesn't exist ", INTERNAL_SERVER_ERROR);
		}
	}
	std::string uploadStore = location->get_uploadStore();
	std::string uploadPath = uploadStore ;
	if (!isDirectory(uploadPath))
	{
		// std::cerr << "Failed to create upload directory: " << uploadPath << std::endl;
		throw HttpException(500, "Internal Server Error - Failed to create upload directory", INTERNAL_SERVER_ERROR);
	}
}

#define MAX_CHUNK_SIZE 8192

void Post::handleFirstCall(HttpRequest *request, std::ofstream &ofs, std::string bounedary)
{
    // this function is called when we have the boundary and the file name
    // we should write the file name and the boundary to the file
    std::cout << "Handling first call for multipart/form-data" << std::endl;
    std::string body = request->GetBodyAsStr();
    size_t pos = body.find("\r\n\r\n");
    ofs.write(body.substr(pos + 4).c_str(), body.size() - pos - 4);
    request->SetRemaineBytes(request->GetBodySize() - (pos + 4));
    request->SetRecvBytes(request->GetRecvBytes() + (pos + 4));  
    std::cout << "Remaining bytes after first call: " << request->GetRemaineBytes() << std::endl;
    std::cout << "Received bytes after first call: " << request->GetRecvBytes() << std::endl;
    // request->SetBodyAsStr("");  
}


void Post::writeToTargetFile(HttpRequest* request, std::string  bounedary, int flag)
{
    /*
        this function gonna have two approaches 
            1- the first call that contain the boundary and the file name so i should base on the crlf to start writing the file
            2- the rest of the calls that will contain the body only so i should write the remaining bytes to the file
    */
    std::cout << "Writing to target file: " << request->GetUploadFilePath() << std::endl;
	std::ofstream ofs(request->GetUploadFilePath(), std::ios::binary | std::ios::app);
    if (!ofs.is_open())
    {
        std::cerr << "Failed to open file for writing: " << request->GetUploadFilePath() << std::endl;
        request->SetIsStreamingUpload(false);
        throw HttpException(500, "Internal Server Error - Failed to open file for writing", INTERNAL_SERVER_ERROR);
    }
    // handling first approach
    if (! flag)
    {
        handleFirstCall(request, ofs, bounedary);
        ofs.close();
        return ;
    }
    // handling the second approach
    // if there is remaining bytes to write form the previous call
    size_t chunk_size = std::min(request->GetRemaineBytes(), static_cast<size_t>(MAX_CHUNK_SIZE));
    
    char buffer[MAX_CHUNK_SIZE];
    size_t byte_readed = recv(request->GetSocketFd(), buffer, chunk_size, 0);
    if (byte_readed < 0)
    {
        std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
        request->SetIsStreamingUpload(false);
        throw HttpException(500, "Internal Server Error - Failed to read from socket", INTERNAL_SERVER_ERROR);
    }
    std::cout << "Bytes readed: " << byte_readed << std::endl;
    ofs.write(buffer, byte_readed);
    request->SetRemaineBytes(request->GetRemaineBytes() - byte_readed);
    request->SetRecvBytes(request->GetRecvBytes() + byte_readed);
    ofs.close();
    if (request->GetRemaineBytes() == 0)
    {
        request->SetIsStreamingUpload(false);
        std::cout << "File upload completed successfully!" << std::endl;
    }
}



void Post::readBodyByChunks(HttpRequest *request)
{
    size_t chunk_size = std::min(request->GetRemaineBytes(), static_cast<size_t>(MAX_CHUNK_SIZE));
    char buffer[MAX_CHUNK_SIZE];

    size_t byte_readed = recv(request->GetSocketFd(), buffer, chunk_size, 0);
    if (byte_readed <= 0) 
    {
        std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
        request->SetIsStreamingUpload(false);
        throw HttpException(500, "Internal Server Error - Failed to read from socket", INTERNAL_SERVER_ERROR);
    }
    
    request->SetBodyAsStr(request->GetBodyAsStr() + std::string(buffer, byte_readed));
    
    // Update counters correctly
    request->SetRemaineBytes(request->GetRemaineBytes() - byte_readed);
    request->SetRecvBytes(request->GetRecvBytes() + byte_readed);    
    std::cout << "Bytes read: " << byte_readed << std::endl;
    std::cout << "Remaining bytes: " << request->GetRemaineBytes() << std::endl;
    std::cout << "Total received: " << request->GetRecvBytes() << std::endl;
    std::cout << "Body size: " << request->GetBodySize() << std::endl;
}

void Post::handleMultipartFormData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    /*
        1 -is it the first time to handle multipart form data
            check if boundary exist and if it's not wait until the next call ;
	*/
    std::cerr << "Handling multipart/form-data size is being approved " << std::endl;
    std::cout << "Location: " << request->GetLocation() << std::endl;

    // std::cout << "--------------------------------" << std::endl;
    // std::cout << request->GetBodyAsStr() << std::endl;
    // std::cout << "--------------------------------" << std::endl;
    // exit(1);
   
    const Location *location = serverConfig.findMatchingLocation(request->GetLocation());
	if (!request->IsStreamingUpload())
    {
        validateLocation(location);
        if (request->HasHeader("Content-Type")  && request->GetBodySize() > location->get_clientMaxBodySize())
        throw HttpException(400, "Payload Too Large", BAD_REQUEST);
        std::string bounedary;
        bounedary = request->GetHeader("Content-Type");
        if (bounedary.find("boundary=") != std::string::npos)
        {
			bounedary = bounedary.substr(bounedary.find("boundary=") + 9);
            request->SetBoundary(bounedary);
        }
        else
        {
            std::cout << "No boundary found in Content-Type header" << std::endl;
			request->SetIsStreamingUpload(false);
            throw HttpException(400, "Bad Request - Boundary not found", BAD_REQUEST);
        }
    }
	// check if their is any boundary in the body then we should look for the file name and try to extract it
	// else i should wait until the next call to handle the multipart form data
	// if it doesn't have any boundary then we should set the request to be a streaming upload
	if (!checkBoundary(request->GetBodyAsStr(), request->GetBoundary()))
	{
		//check if we did need more than on call to have the boundary so i should append the body;
        readBodyByChunks(request);
        request->SetIsStreamingUpload(true);
		return;
	}
	else
	{
        std::cout << "-------- remain bytes:11 " << request->GetRemaineBytes() << std::endl << std::endl << std::endl;
        if (request->GetUploadFilePath().empty())
        {
            std::cout << "Boundary found in body: " << request->GetBoundary() << std::endl;
            // we should extract the file name and the file extension from the body
            std::string file_name = extractFileName(request->GetBodyAsStr(), request->GetBoundary());
            if (file_name.empty())
            {
                request->SetIsStreamingUpload(false);
                throw HttpException(400, "Bad Request - No file name found in body", BAD_REQUEST);
            }
            std::string upload_path = generateUniqueFileName(location->get_uploadStore(), file_name);
            std::cout << "Upload path: " << upload_path << std::endl;
            request->SetUploadFilePath(upload_path);
            writeToTargetFile(request, request->GetBoundary(), 0);
        }
        else
        {
            std::cout << "Continuing to write to target file: " << request->GetUploadFilePath() << std::endl;
            writeToTargetFile(request, request->GetBoundary(), 1);
        }
        std::cout << "remaining bytes: " << request->GetRemaineBytes() << std::endl;
        std::cout << "recv bytes: " << request->GetRecvBytes() << std::endl;
        // exit(1);
        std::cout << "-------- remain bytes:22 " << request->GetRemaineBytes() << std::endl << std::endl << std::endl;
		if (request->GetRemaineBytes() > 0)
        {
            std::cout << "-------- remain bytes: 33" << request->GetRemaineBytes() << std::endl << std::endl << std::endl;
            std::cout << "Remaining bytes to be uploaded: " << request->GetRemaineBytes() << std::endl;
            std::cout << "Continuing to read body by chunks..." << std::endl;
            std::cout << "received bytes: " << request->GetRecvBytes() << std::endl;
            std::cout << "body size: " << request->GetBodySize() << std::endl;
            std::cout << "Content-Length: " << request->GetHeader("Content-Length") << std::endl;
            request->SetIsStreamingUpload(true);
			return ;
        }
		else if (request->GetRemaineBytes() == 0)
		{
			request->SetIsStreamingUpload(false);
			std::cout << "File uploaded successfully to: " << request->GetUploadFilePath() << std::endl;
			request->GetClientDatat()->http_response->setStatusCode(200);
			request->GetClientDatat()->http_response->setStatusMessage("OK");
			request->GetClientDatat()->http_response->setBuffer("File uploaded successfully");
			return;
		}
        else
        {
            std::cout <<"remaining bytes is negative: " << request->GetRemaineBytes() << std::endl;
            exit(1);
        }
	}
}
