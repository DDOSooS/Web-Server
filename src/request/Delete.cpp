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
    if (rel_path.empty())
    {
        std::cerr << "[empty rel_path Not Found ]\n";
        throw HttpException(404, "404 Not Found", NOT_FOUND);
    }
    if (IsValidPath(rel_path) == "") {
        sendErrorResponse(request, 500, "Internal Server Error" , "Not a valid path --> " + rel_path);
    }
    // Check the type if it a directory or a regular file
    std::cout << "✅✅✅ This is a directory to be handled with other function" << std::endl;
    if (IsDir(rel_path)) {
        std::cout << "✅✅✅ This is a directory to be handled with other function" << std::endl;
        sendErrorResponse(request, 409, "Internal Server Error", "Failed to delete file");
    } else { // in case it's a regular file delete it
        if (unlink(rel_path.c_str()) == 0) {
            std::cout << "✅✅✅ Successfully deleted file: " << rel_path << std::endl;
            sendErrorResponse(request, 204, "No Content", "Successfully deleted file: " + rel_path);
        } else {
            std::cout << " ❌ Failed to delete file: " << rel_path << " (errno: " << errno << " - " << strerror(errno) << ")" << std::endl;
            sendErrorResponse(request, 500, "Internal Server Error", "Failed to delete file");
        }
    }







    std::cout << "***************************** [END OF] DELETE REQUEST HANDLER CALLED *****************************************" << std::endl;

}



void Delete::sendSuccessResponse(HttpRequest *request) {
    // Return 204 No Content for successful deletion
    request->GetClientDatat()->http_response->setStatusCode(204);
    request->GetClientDatat()->http_response->setStatusMessage("No Content");
    request->GetClientDatat()->http_response->setBuffer("");
    
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