#include "BitcoinExchange.hpp"

#include <exception>
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Error: could not open file." << std::endl;
        return 1;
    }

    try
    {
        BitcoinExchange exchange;

        exchange.loadDatabase("data.csv");
        exchange.processInputFile(argv[1]);
    }
    catch (const std::exception &exception)
    {
        std::cout << "Error: " << exception.what() << std::endl;
        return 1;
    }

    return 0;
}