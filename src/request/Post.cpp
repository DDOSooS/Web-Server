#include "../../include/request/Post.hpp"
#include <unordered_map>
#include <algorithm>
#include <string.h>

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
    size_t start;
    size_t end;
    
    start = body.find("filename=\"");
    end = body.find("\"", start + 10);
    if (start == std::string::npos || end == std::string::npos)
    {
        std::cerr << "Filename not found in body" << std::endl;
        return "";
    }
    return ( body.substr(start + 10, end - start - 10));
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
}

#define MAX_CHUNK_SIZE 8192

void Post::readBodyChunk(HttpRequest *request)
{
    std::cout << "Reading body chunk for multipart/form-data" << std::endl;
    size_t buffer_size = std::min(request->GetRemaineBytes(), static_cast<size_t>(MAX_CHUNK_SIZE));
    
    char buffer[buffer_size];
    size_t bytesRead = 0;
    bytesRead = recv(request->GetSocketFd(), buffer, buffer_size, 0);
    if (bytesRead < 0)
    {
        std::cerr << "Error reading body chunk: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error - Failed to read body chunk", INTERNAL_SERVER_ERROR);
    }
    else if (bytesRead == 0)
    {
        std::cout << "No more data to read" << std::endl;
        return;
    }
    request->SetBodyAsStr(request->GetBodyAsStr() + std::string(buffer, bytesRead));
}


void Post::getBodyReamiingBytes(HttpRequest *request)
{
    std::cout << "Getting remaining bytes from body" << std::endl;
    size_t pos;

    pos = request->GetBodyAsStr().find("\r\n\r\n");
    if (pos == std::string::npos)
    {
        // unhandled case may be COULD TRIGGER UNEXPECTED BEHAVIOR !! TO TEST IT [TEST!!]
        std::cout << "No remaining bytes found in body" << std::endl;
        return;
    }
    else
    {
        std::fstream file(request->GetUploadFilePath(), std::ios::out | std::ios::binary | std::ios::app);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file for writing: " << request->GetUploadFilePath() << std::endl;
            request->SetIsStreamingUpload(false);
            throw HttpException(500, "Internal Server Error - Failed to open file for writing", INTERNAL_SERVER_ERROR);
        }
        file.write(request->GetBodyAsStr().substr(pos + 4).c_str(), request->GetBodyAsStr().size() - pos - 4);
        if (file.fail())
        {
            std::cout << "Failed to write remaining bytes to file: " << request->GetUploadFilePath() << std::endl;
            exit(1);
        }
        file.close();
        request->SetRemaineBytes( request->GetBodySize() - request->GetBodyAsStr().size());
        request->SetRecvBytes (request->GetBodyAsStr().size());
        request->SetBodyAsStr("");
    }
}
void Post::uploading_logger(HttpRequest *request)
{
    std::cout << "====================---- [UPLOAD LOGGING] START----===========================" << std::endl;
    std::cout << "Uploading file: " << request->GetUploadFilePath() << std::endl;
    std::cout << "Remaining bytes: " << request->GetRemaineBytes() << std::endl;
    std::cout << "Received bytes: " << request->GetRecvBytes() << std::endl;
    std::cout << "Total body size: " << request->GetBodySize() << std::endl;
    std::cout << "Body content: " << request->GetBodyAsStr() << std::endl;
    std::cout << "Uploading prgogress: " << (request->GetRecvBytes() * 100 / request->GetBodySize()) << "%" << std::endl;
    std::cout << "====================---- [UPLOAD LOGGING] END----===========================" << std::endl;
}

void Post::writeToTargetFile(HttpRequest *request, std::string uload_path)
{
    std::fstream file(uload_path, std::ios::out | std::ios::binary | std::ios::app);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for writing: " << uload_path << std::endl;
        request->SetIsStreamingUpload(false);
        throw HttpException(500, "Internal Server Error - Failed to open file for writing", INTERNAL_SERVER_ERROR);
    }
    
    size_t buffer_size = std::min(request->GetRemaineBytes(), static_cast<size_t>(MAX_CHUNK_SIZE));
    char buffer[buffer_size + 1];  // +1 for safety
    size_t byte_read = recv(request->GetSocketFd(), buffer, buffer_size, 0);
    
    if (byte_read < 0)
    {
        request->SetIsStreamingUpload(false);
        std::cerr << "Error reading body chunk: " << strerror(errno) << std::endl;
        throw HttpException(500, "Internal Server Error - Failed to read body chunk", INTERNAL_SERVER_ERROR);
    }
    else if (byte_read == 0)
    {
        std::cout << "No more data to read - connection closed" << std::endl;
        request->SetIsStreamingUpload(false);
        exit(0);
        file.close();
        return;
    }
    
    std::string chunk_data(buffer, byte_read);
    std::string closing_boundary = "\r\n--" + request->GetBoundary() + "--";  // Fixed boundary format
    
    std::cout << "byte_read: " << byte_read << std::endl;
    std::cout << "buffer_size: " << buffer_size << std::endl;
    std::cout << "===================================================\n=================================\n";
    
    size_t boundary_pos = chunk_data.find(closing_boundary);
    if (boundary_pos != std::string::npos)
    {
        // Found closing boundary - write only content before it
        file.write(buffer, boundary_pos);
        
        std::cout << "Upload completed - found closing boundary at position: " << boundary_pos << std::endl;
        std::cout << "Wrote " << boundary_pos << " bytes of file content" << std::endl;
        
        // Update counters: ALL data in this chunk is processed (including boundary)
        request->SetRemaineBytes(request->GetRemaineBytes() - byte_read);
        request->SetRecvBytes(request->GetRecvBytes() + byte_read);
        
        // Upload is complete - stop streaming
        request->SetIsStreamingUpload(false);
        std::cout << "File upload completed!" << std::endl;
    }
    else 
    {
        // No boundary found - write all data and continue streaming
        file.write(buffer, byte_read);
        request->SetRemaineBytes(request->GetRemaineBytes() - byte_read);
        request->SetRecvBytes(request->GetRecvBytes() + byte_read);
        
        std::cout << "Wrote " << byte_read << " bytes, continuing upload..." << std::endl;
    }
    
    file.close();
    uploading_logger(request);
}
 

void Post::handleMultipartFormData(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    /*
        1 -is it the first time to handle multipart form data
            check if boundary exist and if it's not wait until the next call ;
        2- if the boundary is being a valid one 
            = > i should read the body until the reach remaining bytes to 0 
	*/
    // validate Location and also upload store 
    std::cout << "Handling multipart/form-data upload" << std::endl;
    std::cout << "========== [[[ Multipart upload counter: =========" << this->counter << std::endl;
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
            std::cout << "No boundary found in Content-Type header" << std::endl;
			request->SetIsStreamingUpload(false);
            throw HttpException(400, "Bad Request - Boundary not found", BAD_REQUEST);
        }
        request->SetUploadStatus(UPLOAD_BOUNDARY_SEARCH);
    }
    /*
        we do have 2 scenarios here
        -> the body contains the boundary and filename ;
        -> finding the boundary doesn't mean that we have the filename
        -> if it's not we shoul wait until we do find the boundary and filename => then read the body until we reach the end of the body ( remaining bytes = 0 )
    */
    //handling empty body
    if (request->GetUploadingStatus() == UPLOAD_BOUNDARY_SEARCH)
    {
        if (!request->IsStreamingUpload() && request->GetBodyAsStr().empty())
        {
            std::cout << "Waiting for more data to process multipart/form-data!!" << std::endl;
            readBodyChunk(request);
            request->SetIsStreamingUpload(true);
            return;
        }
        else if (request->IsStreamingUpload() && request->GetBodyAsStr().empty() && !request->GetRecvBytes())
        {
            std::cout << "No data to process for multipart/form-data" << std::endl;
            request->SetIsStreamingUpload(false);
            throw HttpException(400, "Bad Request - No data to process", BAD_REQUEST);
            return;
        }
        // hanling the case where we have the boundary and the body is not empty
        if (request->GetBodyAsStr().size() > request->GetBoundary().size())
        {
            std::cout << "Checking boundary in body" << std::endl;
            if (!checkBoundary(request->GetBodyAsStr(), request->GetBoundary()))
            {
                std::cout << "Boundary not found in body" << std::endl;
                std::cout << "boundary: " << request->GetBoundary() << std::endl;
                std::cout << "Body: " << request->GetBodyAsStr() << std::endl;
                exit(1);
                request->SetIsStreamingUpload(false);
                throw HttpException(400, "Bad Request - Boundary not found", BAD_REQUEST);
            }
            else
            {
                std::cout << "Boundary found: " << request->GetBoundary() << std::endl;
                request->SetUploadStatus(UPLOAD_FILENAME_SEARCH);
            }
        }
        else
        {
            std::cout << "Waiting for more data to process multipart/form-data!!!" << std::endl;
            readBodyChunk(request);
            return;
        }
    }
    // we should check for the filename in the body
    if (request->GetUploadingStatus() == UPLOAD_FILENAME_SEARCH)
    {
        if (request->GetBodyAsStr().find("filename=\"") != std::string::npos)
        {
            std::cout << "Extracting file name from body" << std::endl;
            std::string file_name = extractFileName(request->GetBodyAsStr(), request->GetBoundary());
            if (file_name.empty())
            {
                std::cout << "No file name found in body" << std::endl;
                request->SetIsStreamingUpload(false);
                throw HttpException(400, "Bad Request - No file name found", BAD_REQUEST);
            }
            else
            {
                std::cout << "File name extracted: " << file_name << std::endl;
                // generate unique file name
                std::string uploadStore = location->get_uploadStore();
                std::string unique_file_name = generateUniqueFileName(uploadStore, file_name);
                request->SetUploadFilePath(unique_file_name);
                request->SetUploadStatus(UPLOAD_PROCESSING);
            }
        }
        else
        {
            std::cout << "No file name found in body" << std::endl;
            request->SetIsStreamingUpload(false);
            throw HttpException(400, "Bad Request - No file name found", BAD_REQUEST);
        }
    }

    std::cout << "File upload path: " << request->GetUploadFilePath() << std::endl;

    /*
        at this point we are sure that we have a valid boundary and a file name so we can proceed to read the body from the end of the first crlf
        we should read the body until we reach the end of the body ( remaining bytes = 0 )
        first we should process the reamingbody part that exist in request body before starting to read the body in chunks    
    */
    // checking if the body contains any remaining bytes
    if (request->GetUploadingStatus() == UPLOAD_PROCESSING)
    {

        bool flag = false;
        if (!request->GetBodyAsStr().empty())
        {
            flag = true;
            getBodyReamiingBytes(request);
            if (request->GetRemaineBytes() > 0)
            {
                std::cout << "Remaining bytes: " << request->GetRemaineBytes() << std::endl;
                request->SetIsStreamingUpload(true);
                return ;
            }
        }
        uploading_logger(request);
        std::cout << "upload processing function called" << std::endl;
        // exit(1);
        // reading body in chunks
        if (request->GetRemaineBytes() > 0)
            writeToTargetFile(request, request->GetUploadFilePath());
        if (request->GetRemaineBytes() == 0)
        {
            request->SetUploadStatus(UPLOAD_DONE);
            request->SetIsStreamingUpload(false);
            uploading_logger(request);
            std::cout << "UPLOADING IS BEING DONE" << std::endl;
            request->GetClientDatat()->http_response->setStatusCode(200);
            request->GetClientDatat()->http_response->setStatusMessage("OK");
            request->GetClientDatat()->http_response->setBuffer("File uploaded successfully");
            request->GetClientDatat()->http_response->setContentType("text/plain");
            std::cout << "File uploaded successfully" << std::endl;
        }
        else if (request->GetRemaineBytes() > 0 )
        {
            std::cout << "Waiting for more data to process multipart/form-data!!!3" << std::endl;
            request->SetIsStreamingUpload(true);
            request->GetClientDatat()->_server->updatePollEvents(request->GetSocketFd() ,POLLIN);
            return ;
        }
    }
}
