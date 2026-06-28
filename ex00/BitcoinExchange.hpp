#ifndef BITCOINEXCHANGE_HPP
#define BITCOINEXCHANGE_HPP

#include <map>
#include <string>

class BitcoinExchange
{
    private:
        // Store exchange rates indexed by their dates
        std::map<std::string, double> _exchangeRates;

        // Remove whitespace from both ends of a string
        static std::string trim(const std::string &text);

        // Check the format and calendar validity of a date
        static bool isValidDate(const std::string &date);

        // Convert a complete string into a numeric value
        static bool parseNumber(const std::string &text, double &value);

        // Split a line into two non-empty parts
        static bool splitLine(const std::string &line, char separator, std::string &left, std::string &right);

        // Find an exact rate or the closest earlier one
        bool getExchangeRate(const std::string &date, double &rate) const;

        // Validate and process one input record
        void processInputLine(const std::string &line) const;

    public:
        BitcoinExchange();
        BitcoinExchange(const BitcoinExchange &other);
        BitcoinExchange &operator=(const BitcoinExchange &other);
        ~BitcoinExchange();

        // Load exchange rates from the provided CSV database
        void loadDatabase(const std::string &filename);

        // Process every record from the user's input file
        void processInputFile(const std::string &filename) const;
};

#endif