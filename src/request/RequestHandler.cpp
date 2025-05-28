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
RequestHandler * RequestHandler::GetNext(){
    return (_nextHandler);
}

void    RequestHandler::HandleRequest(HttpRequest *request)
{
    // Validate the request
    if (!request)
    {
        std::cerr << "Error: Null request in RequestHandler" << std::endl;
        throw HttpException(500, "Internal Server Error", INTERNAL_SERVER_ERROR);
    }

    std::cout << "RequestHandler::HandleRequest=================" << std::endl;
    
    if (CanHandle(request->GetMethod()))
    {
        std::cout << "RequestHandler::Processing the request================="  << request->GetMethod() << std::endl;
        ProccessRequest(request);
    }
    else if (this->_nextHandler)
    {
        // Forward to the next handler in the chain
        this->_nextHandler->HandleRequest(request);
    }
    else {
        // No handler found for this method
        std::cerr << "No handler for method: " << request->GetMethod() << std::endl;
        throw HttpException(501, "Not Implemented", NOT_IMPLEMENTED);
    }

    return;
}