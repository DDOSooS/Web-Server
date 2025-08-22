#include "../../include/request/Delete.hpp"


Delete::Delete() {

}
Delete::~Delete() {

}

bool Delete::CanHandle(std::string method) {
    return (method == "DELETE");
}


void Delete::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig) {
    std::string rel_path;
    const Location *cur_location;
    (void)serverConfig;
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
    cur_location = clientConfig.findBestMatchingLocation(request->GetLocation());
    rel_path = request->GetRelativePath(cur_location, request->GetClientDatat());
    if (rel_path.empty() || IsValidPath(rel_path) == "")
    {
        std::cerr << "[empty rel_path Not Found ]\n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    if (IsDir(rel_path)) {
        handleDirectoryDeletion(request, rel_path, request->GetLocation());
    } else {
        handleFileDeletion(request, rel_path);
    }





}

void Delete::handleFileDeletion(HttpRequest *request, const std::string &filePath) {
    if (unlink(filePath.c_str()) == 0) {
        sendSuccessResponse(request);
    } else {
        sendErrorResponse(request, 500, "Internal Server Error", "Failed to delete file");
    }
}



void Delete::handleDirectoryDeletion(HttpRequest *request, const std::string &dirPath, const std::string &originalUri) {
    
    if (originalUri.empty() || originalUri.back() != '/') {
        sendErrorResponse(request, 409, "Conflict", "Directory deletion requires URI to end with '/'");
        return;
    }
    
    if (deleteDirectoryRecursive(dirPath)) {
        sendSuccessResponse(request);
    } else {
        sendErrorResponse(request, 500, "Internal Server Error", "Failed to delete directory");
    }
}

bool Delete::deleteDirectoryRecursive(const std::string &dirPath) {
    
    DIR *dir = opendir(dirPath.c_str());
    if (!dir) {
        std::cerr << "Failed to open directory: " << dirPath << std::endl;
        return false;
    }
    
    std::vector<std::string> entries;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        entries.push_back(std::string(entry->d_name));
    }
    closedir(dir);
    
    bool success = true;
    for (std::vector<std::string>::iterator it = entries.begin(); it != entries.end(); ++it) {
        std::string fullPath = dirPath + "/" + *it;
        struct stat st;
        
        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (!deleteDirectoryRecursive(fullPath)) {
                    success = false;
                }
            } else {
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
    
    if (success) {
        if (rmdir(dirPath.c_str()) != 0) {
            std::cerr << "Failed to remove directory: " << dirPath << std::endl;
            success = false;
        }
    }
    
    return success;
}






void Delete::sendSuccessResponse(HttpRequest *request) {
    request->GetClientDatat()->http_response->setStatusCode(204);
    request->GetClientDatat()->http_response->setStatusMessage("No Content");
    request->GetClientDatat()->http_response->setBuffer("File deleted succesfully");
    request->SetProcessed(true);
}


void Delete::sendErrorResponse(HttpRequest *request, int statusCode, const std::string &statusMessage, const std::string &errorMessage) {
    request->GetClientDatat()->http_response->setStatusCode(statusCode);
    request->GetClientDatat()->http_response->setStatusMessage(statusMessage);
    request->GetClientDatat()->http_response->setContentType("text/plain");
    request->GetClientDatat()->http_response->setBuffer(errorMessage);
    request->SetProcessed(true); 
}