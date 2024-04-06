#pragma once

#include <iostream>
#include <unordered_map>
#include <string>

namespace Phi
{
    // Various logging functions to output arguments to console prefixed with severity messages

    template <typename... Args>
    void Log(Args&&... args)
    {
        std::cout << "INFO: ";
        (std::cout << ... << args) << std::endl;
    };

    template <typename... Args>
    void Warning(Args&&... args)
    {
        std::cout << "WARNING: ";
        (std::cout << ... << args) << std::endl;
    };

    template <typename... Args>
    void Error(Args&&... args)
    {
        std::cout << "ERROR: ";
        (std::cout << ... << args) << std::endl;
    };

    template <typename... Args>
    void FatalError(Args&&... args)
    {
        std::cout << "FATAL: ";
        (std::cout << ... << args) << std::endl;
    };
}