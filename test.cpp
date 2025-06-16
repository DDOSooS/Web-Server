#include <unistd.h>
#include <iostream>
#include <limits.h>


int main()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    std::cout << "Current working directory: " << cwd << std::endl;
}