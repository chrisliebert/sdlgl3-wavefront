#ifndef _CONFIG_LOADER_H_
#define _CONFIG_LOADER_H_

#include <string>
#include <string_view>
#include <map>
#include <iostream>

class ConfigLoader
{
protected:
    std::string filename;
    std::map<std::string, std::string> vars;
public:
    // Load a .cfg file
    explicit ConfigLoader(std::string_view configPath);
    ~ConfigLoader() = default;
    
    // Non-copyable, movable
    ConfigLoader(const ConfigLoader&) = delete;
    ConfigLoader& operator=(const ConfigLoader&) = delete;
    ConfigLoader(ConfigLoader&&) = default;
    ConfigLoader& operator=(ConfigLoader&&) = default;
    
    [[nodiscard]] bool getBool(std::string_view key) const;
    [[nodiscard]] int getInt(std::string_view key) const;
    [[nodiscard]] float getFloat(std::string_view key) const;
    [[nodiscard]] std::string_view getVar(std::string_view key) const;
    [[nodiscard]] bool hasVar(std::string_view key) const;
    
    [[nodiscard]] std::string_view getFilename() const { return filename; }
    
    friend std::ostream& operator<<(std::ostream& os, const ConfigLoader& cfg);
};

#endif
