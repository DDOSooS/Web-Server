#include "../../include/request/RequestHandler.hpp"

RequestHandler::RequestHandler():_nextHandler(NULL)
{

}

RequestHandler::~RequestHandler()
{

}

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
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }
    // Check if this request has already been processed
    if (request->IsProcessed())
    {
        return;
    }
    std::string rel_path;
    const Location *cur_location = serverConfig.findMatchingLocation(request->GetLocation());
    rel_path = request->GetRelativePath(cur_location, request->GetClientDatat());
    if (request->IsRedirected())
    {
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
    else
    {
        throw HttpException(501, "Not Implemented", NOT_IMPLEMENTED);
    }
    return;
}
