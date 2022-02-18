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
static inline std::string& ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](int c) {return !std::isspace(c); }));
    return s;
}

// trim from end
static inline std::string& rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](int c) {return !std::isspace(c); }).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string& trim(std::string& s) {
    return ltrim(rtrim(s));
}