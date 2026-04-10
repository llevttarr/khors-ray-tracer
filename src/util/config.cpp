#include <stdexcept>
#include <algorithm>
// #include <boost/program_options.hpp>

#include "config.h"

// namespace po = boost::program_options;

static std::string strip_quotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

Config ConfigParser::parse(std::istream& conf_file) {
    Config conf;

    // po::options_description desc;
    // todo
    return conf;
}
