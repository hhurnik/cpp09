#include "PmergeMe.hpp"

#include <exception>
#include <iostream>

int main(int argc, char **argv)
{
    try
    {
        PmergeMe sorter;

        //parse, display, sort and benchmark the supplied sequence
        sorter.run(argc, argv);
    }
    catch (const std::exception &exception)
    {
        //invalid input and internal validation failures end the program
        std::cerr << exception.what() << std::endl;
        return 1;
    }

    return 0;
}
