#include "RPN.hpp"

#include <climits>
#include <sstream>
#include <stdexcept>

RPN::RPN() {}

RPN::RPN(const RPN &other) : _operands(other._operands) {}

RPN &RPN::operator=(const RPN &other)
{
    // Avoid unnecessary work during self-assignment
    if (this != &other)
        _operands = other._operands;

    return *this;
}

RPN::~RPN() {}

bool RPN::isOperator(const std::string &token)
{
    // Operators must be represented by exactly one character
    if (token.size() != 1)
        return false;

    return (token[0] == '+' || token[0] == '-' || token[0] == '*' || token[0] == '/');
}

void RPN::applyOperator(char operation)
{
    // Every binary operator requires two operands
    if (_operands.size() < 2)
        throw std::runtime_error("Error");

    // The right operand is stored at the top of the stack
    const int right = _operands.top();
    _operands.pop();

    // The next value is the left operand
    const int left = _operands.top();
    _operands.pop();

    // Division by zero is invalid
    if (operation == '/' && right == 0)
        throw std::runtime_error("Error");

    // Reject addition that would overflow an int
    if (operation == '+')
    {
        if ((right > 0 && left > INT_MAX - right) || (right < 0 && left < INT_MIN - right))
        {
            throw std::runtime_error("Error");
        }

        _operands.push(left + right);
        return;
    }

    // Reject subtraction that would overflow an int
    if (operation == '-')
    {
        if ((right > 0 && left < INT_MIN + right) || (right < 0 && left > INT_MAX + right))
        {
            throw std::runtime_error("Error");
        }

        _operands.push(left - right);
        return;
    }

    // Reject multiplication that would overflow an int
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

    // INT_MIN divided by -1 is the only overflowing int division
    if (left == INT_MIN && right == -1)
        throw std::runtime_error("Error");

    _operands.push(left / right);
}

int RPN::evaluate(const std::string &expression)
{
    // Reset the object so repeated calls remain independent
    while (!_operands.empty())
        _operands.pop();

    std::istringstream input(expression);
    std::string token;
    bool hasToken = false;

    while (input >> token)
    {
        hasToken = true;

        // Valid operands are single decimal digits from 0 to 9
        if (token.size() == 1 && token[0] >= '0' && token[0] <= '9')
        {
            _operands.push(token[0] - '0');
            continue;
        }

        // Reject unknown tokens before trying to evaluate them
        if (!isOperator(token))
            throw std::runtime_error("Error");

        applyOperator(token[0]);
    }

    // An empty expression or extra operands are invalid
    if (!hasToken || _operands.size() != 1)
        throw std::runtime_error("Error");

    return _operands.top();
}
