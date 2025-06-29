#include "../../include/request/Delete.hpp"


Delete::Delete() {

}
Delete::~Delete() {

}

// Inherited from RequestHandler
bool Delete::CanHandle(std::string method) {
    std::cout << "***************************** 1] CAN HANDLE DELETE REQUEST *****************************************" << std::endl;
    return (method == "DELETE");
}


void Delete::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig) {
    std::cout << "🛠️ ***************************** [BEGIN] DELETE REQUEST HANDLER CALLED *****************************************" << std::endl;
    std::string rel_path;
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
    // Get the current location from the server configuration
    cur_location = clientConfig.findBestMatchingLocation(request->GetLocation());
    std::cout << "[ Debug ] rel LOCATION: " << request->GetLocation() << std::endl;
    std::cout << "[ Debug ] client location server name: " << clientConfig.get_server_name()<< std::endl;
    std::cout << "[ Debug ] client location server name: " << request->GetClientDatat()->getServerConfig().get_server_name() << std::endl;
    
    rel_path = request->GetRelativePath(cur_location, request->GetClientDatat());
    if (rel_path.empty() || IsValidPath(rel_path) == "")
    {
        std::cerr << "[empty rel_path Not Found ]\n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    // Check the type if it a directory or a regular file
    if (IsDir(rel_path)) {
        std::cout << "✅✅✅ This is a directory to be handled with other function" << std::endl;
        handleDirectoryDeletion(request, rel_path, request->GetLocation());
    } else { // in case it's a regular file delete it
        handleFileDeletion(request, rel_path);
    }





 std::cout << "***************************** [END OF] DELETE REQUEST HANDLER CALLED *****************************************" << std::endl;
}

void Delete::handleFileDeletion(HttpRequest *request, const std::string &filePath) {
    std::cout << "Handling file deletion: " << filePath << std::endl;
    // Try to delete the file
    if (unlink(filePath.c_str()) == 0) {
        std::cout << "Successfully deleted file: " << filePath << std::endl;
        sendSuccessResponse(request);
    } else {
        std::cout << "Failed to delete file: " << filePath << " (errno: " << errno << " - " << strerror(errno) << ")" << std::endl;
        sendErrorResponse(request, 500, "Internal Server Error", "Failed to delete file");
    }
}



void Delete::handleDirectoryDeletion(HttpRequest *request, const std::string &dirPath, const std::string &originalUri) {
    std::cout << "Handling directory deletion: " << dirPath << std::endl;
    std::cout << "Original URI: " << originalUri << std::endl;
    
    // RULE: Directory deletion requires URI to end with '/'
    if (originalUri.empty() || originalUri.back() != '/') {
        std::cout << "Directory deletion requires URI to end with '/'" << std::endl;
        sendErrorResponse(request, 409, "Conflict", "Directory deletion requires URI to end with '/'");
        return;
    }
    
    // Try to delete the directory
    if (deleteDirectoryRecursive(dirPath)) {
        std::cout << "Successfully deleted directory: " << dirPath << std::endl;
        sendSuccessResponse(request);
    } else {
        std::cout << "Failed to delete directory: " << dirPath << std::endl;
        sendErrorResponse(request, 500, "Internal Server Error", "Failed to delete directory");
    }
}

bool Delete::deleteDirectoryRecursive(const std::string &dirPath) {
    std::cout << "Deleting directory recursively: " << dirPath << std::endl;
    
    DIR *dir = opendir(dirPath.c_str());
    if (!dir) {
        std::cerr << "Failed to open directory: " << dirPath << " (errno: " << errno << ")" << std::endl;
        return false;
    }
    
    // Collect all entries first
    std::vector<std::string> entries;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        entries.push_back(std::string(entry->d_name));
    }
    closedir(dir);
    
    // Delete all entries
    bool success = true;
    for (std::vector<std::string>::iterator it = entries.begin(); it != entries.end(); ++it) {
        std::string fullPath = dirPath + "/" + *it;
        struct stat st;
        
        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Recursive directory deletion
                if (!deleteDirectoryRecursive(fullPath)) {
                    success = false;
                }
            } else {
                // Delete file
                if (unlink(fullPath.c_str()) != 0) {
                    std::cerr << "Failed to delete file: " << fullPath << std::endl;
                    success = false;
                }
            }
        } else {
            std::cerr << "Failed to stat: " << fullPath << std::endl;
            success = false;
        }
    }
    
    // Remove the directory itself
    if (success) {
        if (rmdir(dirPath.c_str()) != 0) {
            std::cerr << "Failed to remove directory: " << dirPath << std::endl;
            success = false;
        }
    }
    
    return success;
}






void Delete::sendSuccessResponse(HttpRequest *request) {
    // Return 204 No Content for successful deletion
    request->GetClientDatat()->http_response->setStatusCode(204);
    request->GetClientDatat()->http_response->setStatusMessage("No Content");
    request->GetClientDatat()->http_response->setBuffer("File deleted succesfully");
    
    // Mark request as processed
    request->SetProcessed(true);
    
    std::cout << "Sent 204 No Content response" << std::endl;
}


void Delete::sendErrorResponse(HttpRequest *request, int statusCode, const std::string &statusMessage, const std::string &errorMessage) {
    request->GetClientDatat()->http_response->setStatusCode(statusCode);
    request->GetClientDatat()->http_response->setStatusMessage(statusMessage);
    request->GetClientDatat()->http_response->setContentType("text/plain");
    request->GetClientDatat()->http_response->setBuffer(errorMessage);
    
    // Mark request as processed
    request->SetProcessed(true);
    
    std::cout << "Sent " << statusCode << " " << statusMessage << " response" << std::endl;
}