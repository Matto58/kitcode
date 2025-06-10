#include "parseini.hpp"
#include <iostream>

map<string, string> parseIni(string filename) {
    map<string, string> m{};
    ifstream f(filename);
    string line;
    while (!f.eof()) {
        getline(f, line);
        string key, value;
        bool keyComplete = false;
        for (int i = 0; i < line.length(); i++) {
            if (line[i] == ';') break;
            else if (keyComplete) value += line[i];
            else if (line[i] == '=') keyComplete = true;
            else key += line[i];
        }
        m[key] = value;
    }
    f.close();
    return m;
}