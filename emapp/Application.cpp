//
// Created by sebastian on 1/4/20.
//

#include "Application.h"

#include <iostream>

void Application::Initialize() {
    std::cout << "Initializing application." << std::endl;
}

void Application::SayHello() {
    std::cout << "Hello!" << std::endl;
}

int main(void) {
    std::cout << "Hello World" << std::endl;
    return 0;
}
