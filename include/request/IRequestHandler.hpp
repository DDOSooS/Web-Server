#pragma once 

class IRequestHandler
{
    public:
        virtual IRequestHandler *SetNext(IRequestHandler *next)=0;
        virtual void Handle()=0;
};