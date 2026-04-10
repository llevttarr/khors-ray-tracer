#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <vector>

struct Config{

};

class ConfigParser{
    static Config parse(std::istream& conf_file);
};
#endif
