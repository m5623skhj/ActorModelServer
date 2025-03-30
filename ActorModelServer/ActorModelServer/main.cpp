#include <iostream>
#include "Actor.h"

void FunctionWithEmptyInput()
{
    std::cout << "FunctionWithEmptyInput()" << std::endl;
}

void FunctionWithInputParam(int value, const std::string& text)
{
    std::cout << "FunctionWithInputParam() : value " << value << ", text " << text << std::endl;
}

int main()
{
    Actor actor;
    if (not actor.SendMessage(FunctionWithEmptyInput) ||
        not actor.SendMessage(FunctionWithInputParam, 42, "Hello Actor"))
    {
        std::cout << "SendMessage failed" << std::endl;
        return 0;
    }
    actor.ProcessMessage();

    return 0;
}