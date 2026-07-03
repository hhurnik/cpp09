#include "RPN.hpp"

#include <climits>
#include <cctype>
#include <stdexcept>

RPN::RPN() {}

RPN::RPN(const RPN &other) : _operands(other._operands) {}

RPN &RPN::operator=(const RPN &other)
{
    if (this != &other)
        _operands = other._operands;

    return *this;
}

RPN::~RPN() {}

bool RPN::isOperator(char character)
{
    return (character == '+' || character == '-' || character == '*' || character == '/');
}

void RPN::applyOperator(char operation)
{
    //every binary operator requires two operands
    if (_operands.size() < 2)
        throw std::runtime_error("Error");

    //the right operand is stored at the top of the stack
    const int right = _operands.top();
    _operands.pop();


    const int left = _operands.top();
    _operands.pop();

    if (operation == '/' && right == 0)
        throw std::runtime_error("Error");

    if (operation == '+')
    {
        if ((right > 0 && left > INT_MAX - right) || (right < 0 && left < INT_MIN - right))
        {
            throw std::runtime_error("Error");
        }

        _operands.push(left + right);
        return;
    }

    if (operation == '-')
    {
        if ((right > 0 && left < INT_MIN + right) || (right < 0 && left > INT_MAX + right))
        {
            throw std::runtime_error("Error");
        }

        _operands.push(left - right);
        return;
    }

    if (operation == '*')
    {
        if ((left > 0 && right > 0 && left > INT_MAX / right)
            || (left > 0 && right < 0 && right < INT_MIN / left)
            || (left < 0 && right > 0 && left < INT_MIN / right)
            || (left < 0 && right < 0 && left < INT_MAX / right))
        {
            throw std::runtime_error("Error");
        }

        _operands.push(left * right);
        return;
    }

    //INT_MIN divided by -1 is the only overflowing int division
    if (left == INT_MIN && right == -1)
        throw std::runtime_error("Error");

    _operands.push(left / right);
}

int RPN::evaluate(const std::string &expression)
{
    //reset the stack before starting a new calculation
    while (!_operands.empty())
        _operands.pop();

    bool hasToken = false;

    for (std::string::size_type i = 0; i < expression.size(); ++i)
    {
        //ignore spaces, tabs and other whitespace characters
        if (std::isspace(static_cast<unsigned char>(expression[i])))
            continue;

        const char current = expression[i];
        hasToken = true;

        //every token must contain exactly one character, so the next character must be whitespace or the end
        if (i + 1 < expression.size() && !std::isspace(static_cast<unsigned char>(expression[i + 1])))
        {
            throw std::runtime_error("Error");
        }

        //a number must be a single digit from 0 to 9
        if (current >= '0' && current <= '9')
        {
            _operands.push(current - '0');
            continue;
        }

        //anything other than a digit or valid operator is invalid
        if (!isOperator(current))
            throw std::runtime_error("Error");

        applyOperator(current);
    }

    //the expression cannot be empty and must leave one final result
    if (!hasToken || _operands.size() != 1)
        throw std::runtime_error("Error");

    return _operands.top();
}