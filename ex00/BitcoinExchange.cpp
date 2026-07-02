#include "BitcoinExchange.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <iomanip>

BitcoinExchange::BitcoinExchange()
{
}

BitcoinExchange::BitcoinExchange(const BitcoinExchange &other) : _exchangeRates(other._exchangeRates)
{
}

BitcoinExchange &BitcoinExchange::operator=(const BitcoinExchange &other)
{
    if (this != &other)
        _exchangeRates = other._exchangeRates;

    return *this;
}

BitcoinExchange::~BitcoinExchange()
{
}

std::string BitcoinExchange::trim(const std::string &text)
{
    std::string::size_type begin = 0;
    std::string::size_type end = text.size();

    //find the first non-whitespace character
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
    {
        ++begin;
    }

    //find the last non-whitespace character
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
    {
        --end;
    }

    return text.substr(begin, end - begin);
}

bool BitcoinExchange::splitLine(const std::string &line, char separator, std::string &left, std::string &right)
{
    //find the required separator
    const std::string::size_type separatorPosition = line.find(separator);

    //reject missing or repeated separators
    if (separatorPosition == std::string::npos || line.find(separator, separatorPosition + 1) != std::string::npos)
    {
        return false;
    }

    //extract and clean both parts
    left = trim(line.substr(0, separatorPosition));
    right = trim(line.substr(separatorPosition + 1));

    //both parts must contain a value
    return !left.empty() && !right.empty();
}

bool BitcoinExchange::isValidDate(const std::string &date)
{
    const int daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    //a date must use the exact YYYY-MM-DD format
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
    {
        return false;
    }

    //every character except the separators must be a digit
    for (std::string::size_type i = 0; i < date.size(); ++i)
    {
        if (i == 4 || i == 7)
            continue;

        if (!std::isdigit(static_cast<unsigned char>(date[i])))
            return false;
    }

    // Extract individual date components
    const int year = std::atoi(date.substr(0, 4).c_str());
    const int month = std::atoi(date.substr(5, 2).c_str());
    const int day = std::atoi(date.substr(8, 2).c_str());

    //validate the basic ranges before using the month array
    if (year < 1 || month < 1 || month > 12 || day < 1)
    {
        return false;
    }

    int maximumDay = daysPerMonth[month - 1];

    //apply the Gregorian leap-year rules
    const bool isLeapYear = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);

    if (month == 2 && isLeapYear)
        maximumDay = 29;

    return day <= maximumDay;
}

bool BitcoinExchange::parseNumber(const std::string &text, double &value)
{
    if (text.empty())
        return false;

    std::string::size_type i = 0;

    //accept one optional leading sign
    if (text[i] == '+' || text[i] == '-')
        ++i;

    bool hasDigit = false;

    //read digits before the decimal point
    while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
    {
        hasDigit = true;
        ++i;
    }

    //read an optional decimal part
    if (i < text.size() && text[i] == '.')
    {
        ++i;

        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
        {
            hasDigit = true;
            ++i;
        }
    }

    //at least one digit must be present
    if (!hasDigit)
        return false;

    //reject any remaining invalid characters
    if (i != text.size())
        return false;

    char *end = NULL;

    //reset errno before the conversion
    errno = 0;

    value = std::strtod(text.c_str(), &end);

    //reject failed conversions, overflow and NaN
    if (end == text.c_str() || *end != '\0' || errno == ERANGE || value != value)
    {
        return false;
    }

    return true;
}

void BitcoinExchange::loadDatabase(const std::string &filename)
{
    std::ifstream database(filename.c_str());

    //stop when the historical database cannot be opened
    if (!database.is_open())
        throw std::runtime_error("could not open database file.");

    std::string line;

    //validate the database header
    if (!std::getline(database, line) || trim(line) != "date,exchange_rate")
    {
        throw std::runtime_error("invalid database header.");
    }

    //use a temporary map to avoid storing a partially valid database
    std::map<std::string, double> loadedRates;

    while (std::getline(database, line))
    {
        std::string date;
        std::string rateText;
        double rate = 0.0;

        //validate every part of the database record
        if (!splitLine(line, ',', date, rateText) || !isValidDate(date) || !parseNumber(rateText, rate) || rate < 0.0)
        {
            throw std::runtime_error("invalid database entry: " + trim(line));
        }

        // Insert the date only if it does not already exist - std::map::insert keeps the first value for duplicate keys
        loadedRates.insert(std::make_pair(date, rate));
    }

    //distinguish an input/output failure from a normal end of file -I/O error
    if (database.bad())
        throw std::runtime_error("could not read database file.");

    //a database without any records cannot be used
    if (loadedRates.empty())
        throw std::runtime_error("database is empty.");

    //replace the current map after all records have been validated
    _exchangeRates.swap(loadedRates);
}

bool BitcoinExchange::getExchangeRate(const std::string &date, double &rate) const
{
    if (_exchangeRates.empty())
        return false;

    //find the first date >= the requested date
    std::map<std::string, double>::const_iterator it = _exchangeRates.lower_bound(date);

    //when the exact date exists
    if (it != _exchangeRates.end() && it->first == date)
    {
        rate = it->second;
        return true;
    }

    //no earlier date exists when lower_bound points to begin()
    if (it == _exchangeRates.begin())
        return false;

    //move from the first later date to the closest earlier date
    --it;

    rate = it->second;

    return true;
}

void BitcoinExchange::processInputLine(const std::string &line) const
{
    std::string date;
    std::string valueText;
    double value = 0.0;

    //reject malformed records, dates and numeric values
    if (!splitLine(line, '|', date, valueText) || !isValidDate(date) || !parseNumber(valueText, value))
    {
        std::cout << "Error: bad input => " << trim(line) << std::endl;
        return;
    }

    if (value < 0.0)
    {
        std::cout << "Error: not a positive number."<< std::endl;
        return;
    }

    if (value > 1000.0)
    {
        std::cout << "Error: too large a number." << std::endl;
        return;
    }

    double rate = 0.0;

    //find the exact exchange rate or the closest earlier rate
    if (!getExchangeRate(date, rate))
    {
        std::cout << "Error: no exchange rate available before " << date << "." << std::endl;
        return;
    }

    //print the converted Bitcoin value
    std::cout << std::setprecision(15) << date << " => " << value << " = " << value * rate << std::endl;
}

void BitcoinExchange::processInputFile(const std::string &filename) const
{
    std::ifstream input(filename.c_str());

    if (!input.is_open())
        throw std::runtime_error("could not open file.");

    std::string line;
    bool isFirstLine = true;

    while (std::getline(input, line))
    {
        //skip the expected header when it is the first line
        if (isFirstLine && trim(line) == "date | value")
        {
            isFirstLine = false;
            continue;
        }

        isFirstLine = false;

        //validate and evaluate the current record
        processInputLine(line);
    }

    //report a real reading failure instead of a normal end of file
    if (input.bad())
        throw std::runtime_error("could not read input file.");
}