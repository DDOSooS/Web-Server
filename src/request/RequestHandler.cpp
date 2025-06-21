#include "../../include/request/RequestHandler.hpp"

RequestHandler::RequestHandler():_nextHandler(NULL)
{

}

RequestHandler::~RequestHandler()
{}

RequestHandler *   RequestHandler::SetNext(RequestHandler *handler)
{
    if (handler)
    {
        this->_nextHandler =  handler;
        return handler;
    }
    return this;
}
RequestHandler * RequestHandler::GetNext()
{
    return (_nextHandler);
}


void    RequestHandler::HandleRequest(HttpRequest *request, const ServerConfig &serverConfig, ServerConfig clientConfig)
{
    if (!request)
    {
        std::cerr << "Error: Null request in RequestHandler" << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    
    // Check if this request has already been processed
    if (request->IsProcessed())
    {
        std::cout << "[DEBUG] Request already processed, skipping handler chain" << std::endl;
        return;
    }
    /* check for redirection */
    std::string rel_path;
    // std::cout << "RequestHandler::HandleRequest=================" << std::endl;
    std::cout << "[DEBUG] Handling request for location: " << request->GetLocation() << std::endl;
     const Location *cur_location = serverConfig.findMatchingLocation(request->GetLocation());
    rel_path = request->GetRelativePath(cur_location, request->GetClientDatat());

    if (request->IsRedirected())
    {
        // request->SetRedirectCounter(request->GetRedirectCounter() + 1);
        request->handleRedirect(cur_location, rel_path);
        return;
    }
    if (CanHandle(request->GetMethod()))
    {
        ProccessRequest(request, serverConfig,clientConfig);
    }
    else if (this->_nextHandler)
    {
        this->_nextHandler->HandleRequest(request, serverConfig, clientConfig);
    }
    else {
        std::cerr << "No handler for method: " << request->GetMethod() << std::endl;
        throw HttpException(501, "Not Implemented", NOT_IMPLEMENTED);
    }

    return;
}