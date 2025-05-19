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

void    RequestHandler::HandleRequest(HttpRequest *request)
{
    std::cout << "RequestHandler::HandleRequest=================" << std::endl;
    // exit(1);
    if (CanHandle(request->GetMethod()))
    {
        std::cout << "RequestHandler::Processing the requesr================="  << request->GetMethod() << std::endl;
        // exit(1);
        ProccessRequest(request);
    }
    else if (this->_nextHandler)
    {
        this->HandleRequest(request);
    }

    return ;
}