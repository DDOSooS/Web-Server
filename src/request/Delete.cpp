#include "../../include/request/Delete.hpp"
#include "../../include/request/HttpRequest.hpp"
#include "../../include/ClientConnection.hpp"
#include "../../include/response/HttpResponse.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <limits.h>
#include <vector>

Delete::Delete() {}
Delete::~Delete() {}

bool Delete::CanHandle(std::string method) {
    std::cout << "Delete::CanHandle called with method: '" << method << "'" << std::endl;
    return method == "DELETE";
}

void Delete::ProccessRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig) {
    std::cout << "***** DELETE HANDLER CALLED WITH DEBUG *****" << std::endl;
    (void)serverConfig; // Silence unused parameter warning
    (void)clientConfig; 
    
    // Get the original location and query parameters
    std::string originalLocation = request->GetLocation();
    std::cout << "Original location: '" << originalLocation << "'" << std::endl;
    
    // Extract query string from location if it contains a question mark
    std::string location = originalLocation;
    std::string rawQueryString;
    size_t questionMarkPos = location.find('?');
    
    if (questionMarkPos != std::string::npos) {
        rawQueryString = location.substr(questionMarkPos + 1);
        location = location.substr(0, questionMarkPos);
        request->SetLocation(location); // Update the location in the request object
        std::cout << "Extracted query string from URL: '" << rawQueryString << "'" << std::endl;
        std::cout << "Updated location: '" << location << "'" << std::endl;
    } else {
        // Try to get query string from request object
        rawQueryString = request->GetQueryStringStr();
        std::cout << "Using query string from request object: '" << rawQueryString << "'" << std::endl;
    }
    
    // Get query parameters from vector (usually empty for DELETE requests)
    std::vector<std::pair<std::string, std::string> > queryParams = request->GetQueryString();
    std::cout << "Query params vector size: " << queryParams.size() << std::endl;
    
    // Convert query parameters to map for easier handling
    std::map<std::string, std::string> params;
    
    // Parse from raw query string (this is the most reliable method)
    if (!rawQueryString.empty()) {
        std::cout << "Parsing raw query string manually: '" << rawQueryString << "'" << std::endl;
        params = parseQueryString(rawQueryString);
        std::cout << "Parsed " << params.size() << " parameters from raw string" << std::endl;
        
        for (std::map<std::string, std::string>::iterator it = params.begin(); it != params.end(); ++it) {
            std::cout << "Param: '" << it->first << "' = '" << it->second << "'" << std::endl;
        }
    } 
    
    // Fallback to vector if raw string parsing didn't yield results
    if (params.empty() && !queryParams.empty()) {
        std::cout << "Using query parameters from vector as fallback" << std::endl;
        for (std::vector<std::pair<std::string, std::string> >::iterator it = queryParams.begin(); it != queryParams.end(); ++it) {
            params[it->first] = it->second;
            std::cout << "Vector param: '" << it->first << "' = '" << it->second << "'" << std::endl;
        }
    }
    
    // If we still have no parameters but we have a query string, try a more aggressive parsing approach
    if (params.empty() && !rawQueryString.empty()) {
        std::cout << "Attempting more aggressive query string parsing" << std::endl;
        size_t equalPos = rawQueryString.find('=');
        if (equalPos != std::string::npos) {
            std::string key = urlDecode(rawQueryString.substr(0, equalPos));
            std::string value = urlDecode(rawQueryString.substr(equalPos + 1));
            params[key] = value;
            std::cout << "Parsed parameter using fallback method: '" << key << "' = '" << value << "'" << std::endl;
        }
    }

    // Check if file parameter is provided
    if (params.find("file") == params.end() && params.find("path") == params.end()) {
        std::cout << "No valid parameters found. Available params:" << std::endl;
        for (std::map<std::string, std::string>::iterator it = params.begin(); it != params.end(); ++it) {
            std::cout << "  - '" << it->first << "' = '" << it->second << "'" << std::endl;
        }
        setErrorResponse(request, 400, "Bad Request", 
            "Missing required parameter. Use ?file=filename or ?path=filepath");
        std::cout << "[DEBUG] DELETE handler exiting early - no valid parameters" << std::endl;
        return;
    }

    std::string targetPath;
    if (params.find("file") != params.end()) {
        // Delete file from uploads directory
        std::string filename = params["file"];
        std::cout << "Found file parameter: '" << filename << "'" << std::endl;
        if (!isPathSafe(filename)) {
            setErrorResponse(request, 400, "Bad Request", "Invalid filename");
            std::cout << "[DEBUG] DELETE handler exiting - unsafe filename" << std::endl;
            return;
        }
        targetPath = getUploadsDirectory() + "/" + filename;
    } else {
        // Delete file/directory from specified path
        std::string path = params["path"];
        std::cout << "Found path parameter: '" << path << "'" << std::endl;
        if (!isPathSafe(path)) {
            setErrorResponse(request, 400, "Bad Request", "Invalid path");
            std::cout << "[DEBUG] DELETE handler exiting - unsafe path" << std::endl;
            return;
        }
        // Basic security check - ensure path doesn't escape uploads directory
        if (path.find("..") != std::string::npos || path[0] == '/') {
            setErrorResponse(request, 403, "Forbidden", "Access denied");
            std::cout << "[DEBUG] DELETE handler exiting - forbidden path" << std::endl;
            return;
        }
        targetPath = getUploadsDirectory() + "/" + path;
    }

    std::cout << "Target path for deletion: " << targetPath << std::endl;

    // Check if file/directory exists
    struct stat st;
    if (stat(targetPath.c_str(), &st) == -1) {
        if (errno == ENOENT) {
            std::cout << "File not found: " << targetPath << std::endl;
            setErrorResponse(request, 404, "Not Found", "File or directory not found");
        } else {
            std::cout << "Error accessing file: " << targetPath << " (errno: " << errno << ")" << std::endl;
            setErrorResponse(request, 500, "Internal Server Error", "Failed to access file");
        }
        std::cout << "[DEBUG] DELETE handler exiting - file not found or access error" << std::endl;
        return;
    }

    // Perform deletion
    bool success = false;
    std::string itemType;
    
    if (S_ISDIR(st.st_mode)) {
        itemType = "directory";
        success = deleteDirectory(targetPath);
    } else {
        itemType = "file";
        success = deleteFile(targetPath);
    }

    if (success) {
        // Success response
        std::stringstream responseContent;
        responseContent << "<!DOCTYPE html><html><head><title>Delete Successful</title></head><body>";
        responseContent << "<h1>Delete Successful</h1>";
        responseContent << "<p>The " << itemType << " has been successfully deleted:</p>";
        responseContent << "<p><strong>" << htmlEscape(targetPath) << "</strong></p>";
        responseContent << "<p><a href=\"/\">Go to Home</a> | <a href=\"javascript:history.back()\">Go Back</a></p>";
        responseContent << "</body></html>";
        
        std::string responseBody = responseContent.str();
        
        request->GetClientDatat()->http_response->setStatusCode(200);
        request->GetClientDatat()->http_response->setStatusMessage("OK");
        request->GetClientDatat()->http_response->setContentType("text/html");
        request->GetClientDatat()->http_response->setBuffer(responseBody);
        
        // Mark this request as processed to prevent duplicate handling
        request->SetProcessed(true);
        
        // ADD DEBUG LOGGING:
        std::cout << "[DEBUG] Success response set:" << std::endl;
        std::cout << "  Status Code: 200" << std::endl;
        std::cout << "  Status Message: OK" << std::endl;
        std::cout << "  Content-Type: text/html" << std::endl;
        std::cout << "  Buffer size: " << responseBody.size() << " bytes" << std::endl;
        std::cout << "  Response ready for sending" << std::endl;
        
        std::cout << "[INFO] Successfully deleted " << itemType << ": " << targetPath << std::endl;
    } else {
        setErrorResponse(request, 500, "Internal Server Error", 
            "Failed to delete " + itemType + ". Check permissions.");
        std::cerr << "[ERROR] Failed to delete " << itemType << ": " << targetPath 
                  << " (errno: " << errno << ")" << std::endl;
    }
    
    // CRITICAL: Ensure we exit the function properly
    std::cout << "[DEBUG] DELETE handler completed successfully, returning control" << std::endl;
    return;
}

std::map<std::string, std::string> Delete::parseQueryString(const std::string &queryString) {
    std::map<std::string, std::string> result;
    if (queryString.empty()) {
        std::cout << "Empty query string, nothing to parse" << std::endl;
        return result;
    }
    
    std::cout << "Parsing query string: '" << queryString << "'" << std::endl;
    
    // Trim any leading or trailing whitespace or special characters
    std::string params = queryString;
    while (!params.empty() && (params[0] == ' ' || params[0] == '?' || params[0] == '&')) {
        params.erase(0, 1);
    }
    while (!params.empty() && (params[params.length()-1] == ' ' || params[params.length()-1] == '&')) {
        params.erase(params.length()-1, 1);
    }
    
    if (params.empty()) {
        std::cout << "Query string contained only special characters, nothing to parse" << std::endl;
        return result;
    }
    
    std::cout << "Cleaned query string: '" << params << "'" << std::endl;
    
    // Handle case where there's only one parameter with no '&'
    if (params.find('&') == std::string::npos) {
        size_t equalPos = params.find('=');
        if (equalPos != std::string::npos) {
            std::string key = urlDecode(params.substr(0, equalPos));
            std::string value = urlDecode(params.substr(equalPos + 1));
            result[key] = value;
            std::cout << "Single parameter parsed: '" << key << "' = '" << value << "'" << std::endl;
            return result;
        }
    }
    
    // Split by '&' character for multiple parameters
    std::string pair;
    size_t pos = 0;
    size_t nextPos;
    
    while ((nextPos = params.find('&', pos)) != std::string::npos) {
        pair = params.substr(pos, nextPos - pos);
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, equalPos));
            std::string value = urlDecode(pair.substr(equalPos + 1));
            result[key] = value;
            std::cout << "Parsed parameter: '" << key << "' = '" << value << "'" << std::endl;
        } else if (!pair.empty()) {
            // Handle parameters with no value
            std::string key = urlDecode(pair);
            result[key] = "";
            std::cout << "Parsed parameter with no value: '" << key << "'" << std::endl;
        }
        pos = nextPos + 1;
    }
    
    // Handle the last pair
    if (pos < params.length()) {
        pair = params.substr(pos);
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, equalPos));
            std::string value = urlDecode(pair.substr(equalPos + 1));
            result[key] = value;
            std::cout << "Parsed last parameter: '" << key << "' = '" << value << "'" << std::endl;
        } else if (!pair.empty()) {
            // Handle parameters with no value
            std::string key = urlDecode(pair);
            result[key] = "";
            std::cout << "Parsed last parameter with no value: '" << key << "'" << std::endl;
        }
    }
    
    return result;
}

std::string Delete::urlDecode(const std::string &encoded) {
    std::string result;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hex = encoded.substr(i + 1, 2);
            
            // Validate hex characters
            bool validHex = true;
            for (size_t j = 0; j < hex.length(); ++j) {
                if (!((hex[j] >= '0' && hex[j] <= '9') || 
                      (hex[j] >= 'A' && hex[j] <= 'F') || 
                      (hex[j] >= 'a' && hex[j] <= 'f'))) {
                    validHex = false;
                    break;
                }
            }
            
            if (validHex) {
                std::stringstream ss;
                ss << std::hex << hex;
                int c;
                ss >> c;
                result += static_cast<char>(c);
                i += 2;
            } else {
                // Invalid hex, treat as literal character
                result += encoded[i];
            }
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }
    return result;
}

bool Delete::deleteFile(const std::string &filePath) {
    std::cout << "Attempting to delete file: " << filePath << std::endl;
    
    if (unlink(filePath.c_str()) == 0) {
        std::cout << "Successfully deleted file: " << filePath << std::endl;
        return true;
    } else {
        std::cerr << "Failed to delete file: " << filePath << " (errno: " << errno 
                 << " - " << strerror(errno) << ")" << std::endl;
        return false;
    }
}

bool Delete::deleteDirectory(const std::string &dirPath) {
    std::cout << "Attempting to delete directory: " << dirPath << std::endl;
    
    DIR *dir = opendir(dirPath.c_str());
    if (!dir) {
        std::cerr << "Failed to open directory: " << dirPath << " (errno: " << errno 
                 << " - " << strerror(errno) << ")" << std::endl;
        return false;
    }

    // First, collect all entries to avoid issues with readdir during deletion
    std::vector<std::string> entries;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        entries.push_back(std::string(entry->d_name));
    }
    
    closedir(dir); // Close directory BEFORE attempting deletions
    
    bool success = true;
    
    // Now delete all collected entries
    for (std::vector<std::string>::iterator it = entries.begin(); it != entries.end(); ++it) {
        std::string fullPath = dirPath + "/" + *it;
        struct stat st;
        
        std::cout << "Processing entry: " << fullPath << std::endl;
        
        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                std::cout << "Recursively deleting subdirectory: " << fullPath << std::endl;
                if (!deleteDirectory(fullPath)) {
                    success = false;
                    std::cerr << "Failed to delete subdirectory: " << fullPath << std::endl;
                }
            } else {
                std::cout << "Deleting file: " << fullPath << std::endl;
                if (!deleteFile(fullPath)) {
                    success = false;
                    std::cerr << "Failed to delete file in directory: " << fullPath 
                             << " (errno: " << errno << " - " << strerror(errno) << ")" << std::endl;
                }
            }
        } else {
            std::cerr << "Failed to stat file: " << fullPath << " (errno: " << errno 
                     << " - " << strerror(errno) << ")" << std::endl;
            success = false;
        }
    }
    
    // Remove the directory itself only if all contents were successfully deleted
    if (success) {
        std::cout << "Removing empty directory: " << dirPath << std::endl;
        if (rmdir(dirPath.c_str()) != 0) {
            std::cerr << "Failed to remove directory: " << dirPath << " (errno: " << errno 
                     << " - " << strerror(errno) << ")" << std::endl;
            success = false;
        } else {
            std::cout << "Successfully removed directory: " << dirPath << std::endl;
        }
    } else {
        std::cerr << "Not removing directory " << dirPath << " because some contents failed to delete" << std::endl;
    }
    
    return success;
}

bool Delete::isPathSafe(const std::string &path) {
    // Check for directory traversal attempts
    if (path.find("..") != std::string::npos) {
        std::cerr << "Path contains directory traversal: " << path << std::endl;
        return false;
    }
    
    // Check for absolute paths
    if (!path.empty() && path[0] == '/') {
        std::cerr << "Absolute path not allowed: " << path << std::endl;
        return false;
    }
    
    // Check for null bytes
    if (path.find('\0') != std::string::npos) {
        std::cerr << "Path contains null bytes: " << path << std::endl;
        return false;
    }
    
    // Check for empty path
    if (path.empty()) {
        std::cerr << "Empty path provided" << std::endl;
        return false;
    }
    
    // Additional security checks
    if (path.find("//") != std::string::npos) {
        std::cerr << "Path contains double slashes: " << path << std::endl;
        return false;
    }
    
    // Check for suspicious file names
    if (path == "." || path == ".." || path.find("/.") != std::string::npos) {
        std::cerr << "Suspicious path detected: " << path << std::endl;
        return false;
    }
    
    return true;
}

bool Delete::isDirectoryEmpty(const std::string &dirPath) {
    DIR *dir = opendir(dirPath.c_str());
    if (!dir) {
        return false;
    }
    
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        count++;
        break; // We found at least one entry, no need to continue
    }
    
    closedir(dir);
    return count == 0;
}

std::string Delete::getUploadsDirectory() {
    static std::string uploadsDir;
    if (uploadsDir.empty()) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            uploadsDir = std::string(cwd) + "/uploads";
        } else {
            uploadsDir = "uploads";
        }
        
        // Create uploads directory if it doesn't exist
        struct stat st;
        std::memset(&st, 0, sizeof(st));
        if (stat(uploadsDir.c_str(), &st) == -1) {
            if (mkdir(uploadsDir.c_str(), 0755) != 0) {
                std::cerr << "[ERROR] Failed to create uploads directory: " << uploadsDir
                          << " (errno: " << errno << " - " << strerror(errno) << ")" << std::endl;
            } else {
                std::cout << "[INFO] Created uploads directory: " << uploadsDir << std::endl;
            }
        }
    }
    return uploadsDir;
}

std::string Delete::htmlEscape(const std::string &input) {
    std::string result;
    for (std::string::const_iterator it = input.begin(); it != input.end(); ++it) {
        switch (*it) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += *it; break;
        }
    }
    return result;
}


void Delete::setErrorResponse(HttpRequest *request, int statusCode, const std::string &statusMessage, const std::string &errorMessage) {
    std::stringstream responseContent;
    responseContent << "<!DOCTYPE html><html><head><title>" << statusMessage << "</title></head><body>";
    responseContent << "<h1>" << statusCode << " - " << statusMessage << "</h1>";
    responseContent << "<p>" << htmlEscape(errorMessage) << "</p>";
    responseContent << "<p><a href=\"/\">Go to Home</a> | <a href=\"javascript:history.back()\">Go Back</a></p>";
    responseContent << "</body></html>";
    
    std::string responseBody = responseContent.str();
    
    request->GetClientDatat()->http_response->setStatusCode(statusCode);
    request->GetClientDatat()->http_response->setStatusMessage(statusMessage);
    request->GetClientDatat()->http_response->setContentType("text/html");
    request->GetClientDatat()->http_response->setBuffer(responseBody);
    
    // Mark this request as processed to prevent duplicate handling
    request->SetProcessed(true);
    
    std::cout << "[DEBUG] Error response set:" << std::endl;
    std::cout << "  Status Code: " << statusCode << std::endl;
    std::cout << "  Status Message: " << statusMessage << std::endl;
    std::cout << "  Content-Type: text/html" << std::endl;
    std::cout << "  Buffer size: " << responseBody.size() << " bytes" << std::endl;
    std::cout << "  Error response ready for sending" << std::endl;
}