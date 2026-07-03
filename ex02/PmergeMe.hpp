#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

class PmergeMe
{
    private:
        struct Node
        {
            unsigned int value;
            std::size_t id;

            Node();
            Node(unsigned int number, std::size_t identifier);
            Node(const Node &other);
            Node &operator=(const Node &other);
            ~Node();
        };

        struct Pending
        {
            Node node;
            std::size_t partnerId;
            bool hasPartner;

            //orthodox Canonical Form for the pending-insertion type
            Pending();
            Pending(const Node &valueNode, std::size_t identifier, bool bounded);
            Pending(const Pending &other);
            Pending &operator=(const Pending &other);
            ~Pending();
        };

        //keep the validated input for both container implementations
        std::vector<unsigned int> _input;

        //store independently sorted results for comparison
        std::vector<unsigned int> _vectorResult;
        std::deque<unsigned int> _dequeResult;

        //store measured processing times in microseconds
        double _vectorTime;
        double _dequeTime;

        //parse all command-line arguments as positive integers
        void parseArguments(int argc, char **argv);

        //parse and append every integer contained in one argument
        void parseArgument(const std::string &argument);

        //convert one validated token without risking integer overflow
        static unsigned int parsePositiveInteger(const std::string &token);

        //print the input and the sorted sequence
        void printSequence(const std::string &label, const std::vector<unsigned int> &sequence) const;

        //run and time the vector implementation
        void processVector();

        //run and time the deque implementation
        void processDeque();

        //sort vector nodes with the Ford-Johnson algorithm
        std::vector<Node> fordJohnsonVector(const std::vector<Node> &input) const;

        //insert one pending vector node into its valid search range
        void insertVectorPend(std::vector<Node> &chain, const Pending &pending) const;

        //sort deque nodes with the Ford-Johnson algorithm
        std::deque<Node> fordJohnsonDeque(const std::deque<Node> &input) const;

        //insert one pending deque node into its valid search range
        void insertDequePend(std::deque<Node> &chain, const Pending &pending) const;

        //verify that both implementations produced the same sorted data
        void verifyResults() const;

    public:
        PmergeMe();
        PmergeMe(const PmergeMe &other);
        PmergeMe &operator=(const PmergeMe &other);
        ~PmergeMe();

        //parse, sort and display the complete program result
        void run(int argc, char **argv);
};

#endif
