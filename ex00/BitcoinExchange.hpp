#ifndef BITCOINEXCHANGE_HPP
#define BITCOINEXCHANGE_HPP

#include <map>
#include <string>

class BitcoinExchange
{
    private:
        std::map<std::string, double> _exchangeRates;

        //remove whitespace from both ends of a string
        static std::string trim(const std::string &text);
        
        static bool isValidDate(const std::string &date);

        //convert a complete string into a numeric value
        static bool parseNumber(const std::string &text, double &value);

        //split a line into two non-empty parts
        static bool splitLine(const std::string &line, char separator, std::string &left, std::string &right);

        //find an exact rate or the closest earlier one
        bool getExchangeRate(const std::string &date, double &rate) const;

        //validate and process one input record
        void processInputLine(const std::string &line) const;

    public:
        BitcoinExchange();
        BitcoinExchange(const BitcoinExchange &other);
        BitcoinExchange &operator=(const BitcoinExchange &other);
        ~BitcoinExchange();

        //load exchange rates from the provided CSV database
        void loadDatabase(const std::string &filename);

        //process every record from the user's input file
        void processInputFile(const std::string &filename) const;
};

#endif