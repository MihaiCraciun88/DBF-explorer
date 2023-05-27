#pragma once

#include <filesystem>
#include <locale>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

static inline json getDirFiles(std::string path)
{
    json files = json::array();
    for (auto& filepath : std::filesystem::directory_iterator(path))
    {
        const std::wstring wsfile = filepath.path();
        const std::string file(wsfile.begin(), wsfile.end());
        files.push_back(file);
    }
    return files;
}

// trim from start
static inline std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](int c) {return !std::isspace(c); }));
    return s;
}

// trim from end
static inline std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](int c) {return !std::isspace(c); }).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}

static inline std::vector<std::string> split(std::string text, std::string delim)
{
    std::vector<std::string> list;
    size_t pos = 0, prevPos = 0;
    do {
        pos = text.find(delim, prevPos);
        if (pos == std::string::npos)
        {
            list.push_back(text.substr(prevPos, text.size() - prevPos));
        }
        else
        {
            list.push_back(text.substr(prevPos, pos - prevPos));
            prevPos = pos + delim.length();
        }
    } while (pos != std::string::npos);
    return list;
}

static inline void printVectorString(const std::vector<std::string>& vector)
{
    for (const std::string& item : vector)
    {
        printf("%s\n", item.c_str());
    }
}

static inline json urlEncodedToJson(std::string text)
{
    std::vector<std::string> params = split(text, "&");
    json result = json::object();
    for (std::string& param : params)
    {
        std::vector<std::string> vars = split(param, "=");
        result[vars.at(0).c_str()] = vars.at(1).c_str();
    }
    return result;
}