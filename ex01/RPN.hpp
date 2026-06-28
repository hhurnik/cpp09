#ifndef RPN_HPP
#define RPN_HPP

#include <stack>
#include <string>

class RPN
{
    private:
        // Store operands until an operator consumes them
        std::stack<int> _operands;

        // Check whether a token is one of the supported operators
        static bool isOperator(const std::string &token);

        // Apply one operator to the top two stack values
        void applyOperator(char operation);
        
    public:
        RPN();
        RPN(const RPN &other);
        RPN &operator=(const RPN &other);
        ~RPN();

        // Evaluate a complete Reverse Polish Notation expression
        int evaluate(const std::string &expression);
};

#endif
