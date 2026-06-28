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
    // Avoid unnecessary work during self-assignment
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

    // Find the first non-whitespace character
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
    {
        ++begin;
    }

    // Find the last non-whitespace character
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
    {
        --end;
    }

    return text.substr(begin, end - begin);
}

bool BitcoinExchange::splitLine(const std::string &line, char separator, std::string &left, std::string &right)
{
    // Find the required separator
    const std::string::size_type separatorPosition = line.find(separator);

    // Reject missing or repeated separators
    if (separatorPosition == std::string::npos || line.find(separator, separatorPosition + 1) != std::string::npos)
    {
        return false;
    }

    // Extract and clean both parts
    left = trim(line.substr(0, separatorPosition));
    right = trim(line.substr(separatorPosition + 1));

    // Both parts must contain a value
    return !left.empty() && !right.empty();
}

bool BitcoinExchange::isValidDate(const std::string &date)
{
    const int daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // A date must use the exact YYYY-MM-DD format
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
    {
        return false;
    }

    // Every character except the separators must be a digit
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

    // Validate the basic ranges before using the month array
    if (year < 1 || month < 1 || month > 12 || day < 1)
    {
        return false;
    }

    int maximumDay = daysPerMonth[month - 1];

    // Apply the Gregorian leap-year rules
    const bool isLeapYear = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);

    // February has 29 days during a leap year
    if (month == 2 && isLeapYear)
        maximumDay = 29;

    return day <= maximumDay;
}

bool BitcoinExchange::parseNumber(const std::string &text, double &value)
{
    if (text.empty())
        return false;

    std::string::size_type i = 0;

    // Accept one optional leading sign
    if (text[i] == '+' || text[i] == '-')
        ++i;

    bool hasDigit = false;

    // Read digits before the decimal point.
    while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
    {
        hasDigit = true;
        ++i;
    }

    // Read an optional decimal part
    if (i < text.size() && text[i] == '.')
    {
        ++i;

        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
        {
            hasDigit = true;
            ++i;
        }
    }

    // At least one digit must be present
    if (!hasDigit)
        return false;

    // Read an optional scientific-notation exponent
    if (i < text.size() && (text[i] == 'e' || text[i] == 'E'))
    {
        ++i;

        // The exponent can have its own sign
        if (i < text.size() && (text[i] == '+' || text[i] == '-'))
        {
            ++i;
        }

        const std::string::size_type exponentBegin = i;

        // Read the exponent digits
        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
        {
            ++i;
        }

        // An exponent marker must be followed by digits
        if (i == exponentBegin)
            return false;
    }

    // Reject any remaining invalid characters
    if (i != text.size())
        return false;

    char *end = NULL;

    // Reset errno before the conversion
    errno = 0;

    value = std::strtod(text.c_str(), &end);

    // Reject failed conversions, overflow and NaN
    if (end == text.c_str() || *end != '\0' || errno == ERANGE || value != value)
    {
        return false;
    }

    return true;
}

void BitcoinExchange::loadDatabase(const std::string &filename)
{
    std::ifstream database(filename.c_str());

    // Stop when the historical database cannot be opened
    if (!database.is_open())
        throw std::runtime_error("could not open database file.");

    std::string line;

    // Validate the database header
    if (!std::getline(database, line) || trim(line) != "date,exchange_rate")
    {
        throw std::runtime_error("invalid database header.");
    }

    // Use a temporary map to avoid storing a partially valid database
    std::map<std::string, double> loadedRates;

    while (std::getline(database, line))
    {
        std::string date;
        std::string rateText;
        double rate = 0.0;

        // Validate every part of the database record
        if (!splitLine(line, ',', date, rateText) || !isValidDate(date) || !parseNumber(rateText, rate) || rate < 0.0)
        {
            throw std::runtime_error("invalid database entry: " + trim(line));
        }

        // Insert keeps the dates sorted inside the map
        const std::pair<std::map<std::string, double>::iterator, bool > result = loadedRates.insert(std::make_pair(date, rate));

        // Reject duplicate dates instead of replacing their rates
        if (!result.second)
        {
            throw std::runtime_error(
                "duplicate date in database: " + date
            );
        }
    }

    // Distinguish an input/output failure from a normal end of file
    if (database.bad())
        throw std::runtime_error("could not read database file.");

    // A database without any records cannot be used
    if (loadedRates.empty())
        throw std::runtime_error("database is empty.");

    // Replace the current map after all records have been validated
    _exchangeRates.swap(loadedRates);
}

bool BitcoinExchange::getExchangeRate(const std::string &date, double &rate) const
{
    if (_exchangeRates.empty())
        return false;

    // Find the first date greater than or equal to the requested date
    std::map<std::string, double>::const_iterator it = _exchangeRates.lower_bound(date);

    // Use the rate directly when the exact date exists
    if (it != _exchangeRates.end() && it->first == date)
    {
        rate = it->second;
        return true;
    }

    // No earlier date exists when lower_bound points to begin()
    if (it == _exchangeRates.begin())
        return false;

    // Move from the first later date to the closest earlier date
    --it;

    rate = it->second;

    return true;
}

void BitcoinExchange::processInputLine(const std::string &line) const
{
    std::string date;
    std::string valueText;
    double value = 0.0;

    // Reject malformed records, dates and numeric values
    if (!splitLine(line, '|', date, valueText) || !isValidDate(date) || !parseNumber(valueText, value))
    {
        std::cout << "Error: bad input => " << trim(line) << std::endl;
        return;
    }

    // Reject negative values, including textual negative zero
    if (value < 0.0)
    {
        std::cout << "Error: not a positive number."<< std::endl;
        return;
    }

    // The subject limits input values to 1000
    if (value > 1000.0)
    {
        std::cout << "Error: too large a number." << std::endl;
        return;
    }

    double rate = 0.0;

    // Find the exact exchange rate or the closest earlier rate
    if (!getExchangeRate(date, rate))
    {
        std::cout << "Error: no exchange rate available before " << date << "." << std::endl;
        return;
    }

    // Print the converted Bitcoin value
    std::cout << std::setprecision(15) << date << " => " << value << " = " << value * rate << std::endl;
}

void BitcoinExchange::processInputFile(const std::string &filename) const
{
    std::ifstream input(filename.c_str());

    // Stop when the user input file cannot be opened
    if (!input.is_open())
        throw std::runtime_error("could not open file.");

    std::string line;
    bool isFirstLine = true;

    while (std::getline(input, line))
    {
        // Skip the expected header when it is the first line
        if (isFirstLine && trim(line) == "date | value")
        {
            isFirstLine = false;
            continue;
        }

        isFirstLine = false;

        // Validate and evaluate the current record
        processInputLine(line);
    }

    // Report a real reading failure instead of a normal end of file
    if (input.bad())
        throw std::runtime_error("could not read input file.");
}