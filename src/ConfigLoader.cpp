#include "Common.h"
#include "ConfigLoader.h"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <fstream>

static std::string trimWhitespace(const std::string& s)
{
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

ConfigLoader::ConfigLoader(std::string_view configPath)
    : filename(configPath)
{
    const std::filesystem::path filePath = std::filesystem::path(CONFIG_DIRECTORY_STR) / std::filesystem::path(configPath);
    
    std::ifstream fileStream(filePath.string().c_str());
    if (fileStream.is_open())
    {
        std::cout << "loaded " << filePath << std::endl;
        std::string line;
        while (std::getline(fileStream, line))
        {
            // Remove carriage return for cross-platform support
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            
            // Skip comments and empty lines
            auto trimmed = trimWhitespace(line);
            if (trimmed.empty() || trimmed[0] == '#') continue;
            
            // Find the equals sign
            auto eqPos = trimmed.find('=');
            if (eqPos == std::string::npos) continue;
            
            const std::string key = trimWhitespace(trimmed.substr(0, eqPos));
            const std::string value = trimWhitespace(trimmed.substr(eqPos + 1));
            
            if (!key.empty()) {
                vars[key] = value;
            }
        }
        fileStream.close();
    }
    else
    {
        std::cerr << "Unable to load config: " << filePath << std::endl;
    }
}

bool ConfigLoader::getBool(std::string_view key) const
{
    std::string keyStr(key);
    auto it = vars.find(keyStr);
    if (it == vars.end())
    {
        return false;
    }
    
    const std::string& s = it->second;
    if (s == "true" || s == "True" || s == "1") return true;
    if (s == "false" || s == "False" || s == "0") return false;
    
    std::stringstream ss(s);
    bool val;
    if (ss >> val && ss.fail()) {
        std::cerr << "Unable to parse variable " << key << " of " << s << " as bool." << std::endl;
    }
    return val;
}

int ConfigLoader::getInt(std::string_view key) const
{
    std::string keyStr(key);
    auto it = vars.find(keyStr);
    if (it == vars.end())
    {
        return 0;
    }
    
    const std::string& s = it->second;
    try {
        return std::stoi(s);
    } catch (const std::exception& e) {
        std::cerr << "Unable to parse variable " << key << " of " << s << " as integer: " << e.what() << std::endl;
        return 0;
    }
}

float ConfigLoader::getFloat(std::string_view key) const
{
    std::string keyStr(key);
    auto it = vars.find(keyStr);
    if (it == vars.end())
    {
        return 0.0f;
    }
    
    const std::string& s = it->second;
    try {
        return std::stof(s);
    } catch (const std::exception& e) {
        std::cerr << "Unable to parse variable " << key << " of " << s << " as float: " << e.what() << std::endl;
        return 0.0f;
    }
}

std::string_view ConfigLoader::getVar(std::string_view key) const
{
    static constexpr std::string_view emptyStr{};
    
    std::string keyStr(key);
    auto it = vars.find(keyStr);
    if (it == vars.end())
    {
        return emptyStr;
    }
    return it->second;
}

bool ConfigLoader::hasVar(std::string_view key) const
{
    std::string keyStr(key);
    return vars.find(keyStr) != vars.end();
}

std::ostream& operator<<(std::ostream& os, const ConfigLoader& cfg)
{
    os << "|------- " << cfg.filename << " -------|" << std::endl;
    for (const auto& [key, value] : cfg.vars) {
        os << key << " <- " << value << '\n';
    }
    os << "|--------";
    for (std::size_t i = 0; i < cfg.filename.length(); ++i) {
        os << "-";
    }
    os << "--------|" << std::endl;
    return os;
}
