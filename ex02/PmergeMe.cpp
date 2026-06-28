#include "PmergeMe.hpp"

#include <climits>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

PmergeMe::Node::Node() : value(0), id(0) {}

PmergeMe::Node::Node(unsigned int number, std::size_t identifier) : value(number), id(identifier) {}

PmergeMe::Node::Node(const Node &other) : value(other.value), id(other.id) {}

PmergeMe::Node &PmergeMe::Node::operator=(const Node &other)
{
    // Avoid unnecessary work during self-assignment
    if (this != &other)
    {
        value = other.value;
        id = other.id;
    }

    return *this;
}

PmergeMe::Node::~Node() {}

PmergeMe::Pending::Pending() : node(), partnerId(0), hasPartner(false) {}

PmergeMe::Pending::Pending(const Node &valueNode, std::size_t identifier, bool bounded)
    : node(valueNode), partnerId(identifier), hasPartner(bounded) {}

PmergeMe::Pending::Pending(const Pending &other)
    : node(other.node), partnerId(other.partnerId), hasPartner(other.hasPartner) {}

PmergeMe::Pending &PmergeMe::Pending::operator=(const Pending &other)
{
    if (this != &other)
    {
        node = other.node;
        partnerId = other.partnerId;
        hasPartner = other.hasPartner;
    }

    return *this;
}

PmergeMe::Pending::~Pending() {}

PmergeMe::PmergeMe() : _vectorTime(0.0), _dequeTime(0.0) {}

PmergeMe::PmergeMe(const PmergeMe &other)
    : _input(other._input),
      _vectorResult(other._vectorResult),
      _dequeResult(other._dequeResult),
      _vectorTime(other._vectorTime),
      _dequeTime(other._dequeTime) {}

PmergeMe &PmergeMe::operator=(const PmergeMe &other)
{
    // Avoid unnecessary work during self-assignment
    if (this != &other)
    {
        _input = other._input;
        _vectorResult = other._vectorResult;
        _dequeResult = other._dequeResult;
        _vectorTime = other._vectorTime;
        _dequeTime = other._dequeTime;
    }

    return *this;
}

PmergeMe::~PmergeMe() {}

unsigned int PmergeMe::parsePositiveInteger(const std::string &token)
{
    // Empty tokens cannot represent integers
    if (token.empty())
        throw std::runtime_error("Error");

    unsigned long value = 0;

    for (std::string::size_type i = 0; i < token.size(); ++i)
    {
        // Reject signs, decimal points and every non-digit character
        if (token[i] < '0' || token[i] > '9')
            throw std::runtime_error("Error");

        const unsigned long digit = token[i] - '0';

        // Reject values that cannot fit into a signed positive integer
        if (value > (static_cast<unsigned long>(INT_MAX) - digit) / 10)
            throw std::runtime_error("Error");

        value = value * 10 + digit;
    }

    // The subject requires strictly positive integers
    if (value == 0)
        throw std::runtime_error("Error");

    return static_cast<unsigned int>(value);
}

void PmergeMe::parseArgument(const std::string &argument)
{
    std::istringstream input(argument);
    std::string token;
    bool foundToken = false;

    // Supporting whitespace inside one argument makes input handling robust
    while (input >> token)
    {
        foundToken = true;
        _input.push_back(parsePositiveInteger(token));
    }

    // Reject empty or whitespace-only arguments
    if (!foundToken)
        throw std::runtime_error("Error");
}

void PmergeMe::parseArguments(int argc, char **argv)
{
    _input.clear();

    // At least one positive integer must be supplied
    if (argc < 2)
        throw std::runtime_error("Error");

    for (int i = 1; i < argc; ++i)
        parseArgument(argv[i]);

    if (_input.empty())
        throw std::runtime_error("Error");
}

void PmergeMe::printSequence(const std::string &label, const std::vector<unsigned int> &sequence) const
{
    std::cout << label;

    for (std::vector<unsigned int>::const_iterator it = sequence.begin();
         it != sequence.end(); ++it)
    {
        std::cout << " " << *it;
    }

    std::cout << std::endl;
}

void PmergeMe::insertVectorPend(std::vector<Node> &chain, const Pending &pending) const
{
    std::size_t right = chain.size();

    // Paired values only need to search before their larger partner
    if (pending.hasPartner)
    {
        right = 0;

        while (right < chain.size() && chain[right].id != pending.partnerId)
        {
            ++right;
        }

        if (right == chain.size())
            throw std::runtime_error("Error");
    }

    std::size_t left = 0;

    // Use an upper-bound binary search inside the valid prefix
    while (left < right)
    {
        const std::size_t middle = left + (right - left) / 2;

        if (pending.node.value < chain[middle].value)
            right = middle;
        else
            left = middle + 1;
    }

    chain.insert(chain.begin() + left, pending.node);
}

std::vector<PmergeMe::Node> PmergeMe::fordJohnsonVector(const std::vector<Node> &input) const
{
    // A sequence of zero or one element is already sorted
    if (input.size() <= 1)
        return input;

    std::vector<Node> largerValues;
    largerValues.reserve(input.size() / 2);

    // Index smaller partners by the unique ID of their larger value
    std::vector<Node> smallerByLargerId(_input.size());
    std::vector<char> hasSmallerPartner(_input.size(), 0);

    for (std::size_t i = 0; i + 1 < input.size(); i += 2)
    {
        Node smaller;
        Node larger;

        // Compare each pair and separate its smaller and larger values
        if (input[i].value <= input[i + 1].value)
        {
            smaller = input[i];
            larger = input[i + 1];
        }
        else
        {
            smaller = input[i + 1];
            larger = input[i];
        }

        largerValues.push_back(larger);
        smallerByLargerId[larger.id] = smaller;
        hasSmallerPartner[larger.id] = 1;
    }

    // Recursively sort the larger half of every pair
    std::vector<Node> mainChain = fordJohnsonVector(largerValues);
    std::vector<Pending> pending;
    pending.reserve(mainChain.size() + input.size() % 2);

    for (std::size_t i = 0; i < mainChain.size(); ++i)
    {
        if (!hasSmallerPartner[mainChain[i].id])
            throw std::runtime_error("Error");

        Pending entry;
        entry.node = smallerByLargerId[mainChain[i].id];
        entry.partnerId = mainChain[i].id;
        entry.hasPartner = true;
        pending.push_back(entry);
    }

    // An odd unpaired value is inserted without a partner bound
    if (input.size() % 2 != 0)
    {
        Pending entry;
        entry.node = input[input.size() - 1];
        entry.partnerId = 0;
        entry.hasPartner = false;
        pending.push_back(entry);
    }

    if (pending.empty())
        return mainChain;

    // The first smaller value is already known to precede its partner
    mainChain.insert(mainChain.begin(), pending[0].node);

    std::size_t insertedUntil = 1;
    std::size_t jacobsthalPrevious = 1;
    std::size_t jacobsthalCurrent = 3;

    // Insert pending values in reverse Jacobsthal groups
    while (insertedUntil < pending.size())
    {
        std::size_t groupEnd = jacobsthalCurrent;

        if (groupEnd > pending.size())
            groupEnd = pending.size();

        for (std::size_t index = groupEnd; index > insertedUntil; --index)
            insertVectorPend(mainChain, pending[index - 1]);

        insertedUntil = groupEnd;

        const std::size_t nextJacobsthal =
            jacobsthalCurrent + 2 * jacobsthalPrevious;

        jacobsthalPrevious = jacobsthalCurrent;
        jacobsthalCurrent = nextJacobsthal;
    }

    return mainChain;
}

void PmergeMe::processVector()
{
    const std::clock_t start = std::clock();
    std::vector<Node> nodes;
    nodes.reserve(_input.size());

    // Build vector nodes with unique IDs for pair tracking
    for (std::size_t i = 0; i < _input.size(); ++i)
    {
        Node node;
        node.value = _input[i];
        node.id = i;
        nodes.push_back(node);
    }

    const std::vector<Node> sorted = fordJohnsonVector(nodes);
    _vectorResult.clear();
    _vectorResult.reserve(sorted.size());

    // Convert internal nodes back to the required integer sequence
    for (std::size_t i = 0; i < sorted.size(); ++i)
        _vectorResult.push_back(sorted[i].value);

    const std::clock_t end = std::clock();

    _vectorTime = static_cast<double>(end - start) * 1000000.0 / static_cast<double>(CLOCKS_PER_SEC);
}

void PmergeMe::insertDequePend(std::deque<Node> &chain, const Pending &pending) const
{
    std::size_t right = chain.size();

    // Paired values only need to search before their larger partner
    if (pending.hasPartner)
    {
        right = 0;

        while (right < chain.size() && chain[right].id != pending.partnerId)
        {
            ++right;
        }

        if (right == chain.size())
            throw std::runtime_error("Error");
    }

    std::size_t left = 0;

    // Use an upper-bound binary search inside the valid prefix
    while (left < right)
    {
        const std::size_t middle = left + (right - left) / 2;

        if (pending.node.value < chain[middle].value)
            right = middle;
        else
            left = middle + 1;
    }

    chain.insert(chain.begin() + left, pending.node);
}

std::deque<PmergeMe::Node> PmergeMe::fordJohnsonDeque(const std::deque<Node> &input) const
{
    // A sequence of zero or one element is already sorted
    if (input.size() <= 1)
        return input;

    std::deque<Node> largerValues;

    // Index smaller partners by the unique ID of their larger value
    std::deque<Node> smallerByLargerId(_input.size());
    std::deque<char> hasSmallerPartner(_input.size(), 0);

    for (std::size_t i = 0; i + 1 < input.size(); i += 2)
    {
        Node smaller;
        Node larger;

        // Compare each pair and separate its smaller and larger values
        if (input[i].value <= input[i + 1].value)
        {
            smaller = input[i];
            larger = input[i + 1];
        }
        else
        {
            smaller = input[i + 1];
            larger = input[i];
        }

        largerValues.push_back(larger);
        smallerByLargerId[larger.id] = smaller;
        hasSmallerPartner[larger.id] = 1;
    }

    // Recursively sort the larger half of every pair
    std::deque<Node> mainChain = fordJohnsonDeque(largerValues);
    std::deque<Pending> pending;

    for (std::size_t i = 0; i < mainChain.size(); ++i)
    {
        if (!hasSmallerPartner[mainChain[i].id])
            throw std::runtime_error("Error");

        Pending entry;
        entry.node = smallerByLargerId[mainChain[i].id];
        entry.partnerId = mainChain[i].id;
        entry.hasPartner = true;
        pending.push_back(entry);
    }

    // An odd unpaired value is inserted without a partner bound
    if (input.size() % 2 != 0)
    {
        Pending entry;
        entry.node = input[input.size() - 1];
        entry.partnerId = 0;
        entry.hasPartner = false;
        pending.push_back(entry);
    }

    if (pending.empty())
        return mainChain;

    // The first smaller value is already known to precede its partner
    mainChain.push_front(pending[0].node);

    std::size_t insertedUntil = 1;
    std::size_t jacobsthalPrevious = 1;
    std::size_t jacobsthalCurrent = 3;

    // Insert pending values in reverse Jacobsthal groups
    while (insertedUntil < pending.size())
    {
        std::size_t groupEnd = jacobsthalCurrent;

        if (groupEnd > pending.size())
            groupEnd = pending.size();

        for (std::size_t index = groupEnd; index > insertedUntil; --index)
            insertDequePend(mainChain, pending[index - 1]);

        insertedUntil = groupEnd;

        const std::size_t nextJacobsthal = jacobsthalCurrent + 2 * jacobsthalPrevious;

        jacobsthalPrevious = jacobsthalCurrent;
        jacobsthalCurrent = nextJacobsthal;
    }

    return mainChain;
}

void PmergeMe::processDeque()
{
    const std::clock_t start = std::clock();
    std::deque<Node> nodes;

    // Build deque nodes with unique IDs for pair tracking
    for (std::size_t i = 0; i < _input.size(); ++i)
    {
        Node node;
        node.value = _input[i];
        node.id = i;
        nodes.push_back(node);
    }

    const std::deque<Node> sorted = fordJohnsonDeque(nodes);
    _dequeResult.clear();

    // Convert internal nodes back to the required integer sequence
    for (std::size_t i = 0; i < sorted.size(); ++i)
        _dequeResult.push_back(sorted[i].value);

    const std::clock_t end = std::clock();

    _dequeTime = static_cast<double>(end - start) * 1000000.0 / static_cast<double>(CLOCKS_PER_SEC);
}

void PmergeMe::verifyResults() const
{
    // Both containers must contain exactly the same number of values
    if (_vectorResult.size() != _dequeResult.size())
        throw std::runtime_error("Error");

    for (std::size_t i = 0; i < _vectorResult.size(); ++i)
    {
        // Both independent implementations must produce equal results
        if (_vectorResult[i] != _dequeResult[i])
            throw std::runtime_error("Error");

        // Verify that the resulting sequence is non-decreasing
        if (i > 0 && _vectorResult[i - 1] > _vectorResult[i])
            throw std::runtime_error("Error");
    }
}

void PmergeMe::run(int argc, char **argv)
{
    parseArguments(argc, argv);

    printSequence("Before:", _input);

    processVector();
    processDeque();
    verifyResults();

    printSequence("After:", _vectorResult);

    // Use enough precision to make the timing difference visible
    std::cout << std::fixed << std::setprecision(5);
    std::cout << "Time to process a range of " << _input.size() << " elements with std::vector : "
              << _vectorTime << " us" << std::endl;
    std::cout << "Time to process a range of " << _input.size() << " elements with std::deque  : "
              << _dequeTime << " us" << std::endl;
}
