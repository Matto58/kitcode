#include "config.hpp"
#include "parseini.hpp"
#include <iostream>

bool loadConfig(string filename, kcconfig *config) {
    if (config == NULL) return false;
    map<string, string> m = parseIni(filename);
    string line;
    for (auto &p : m) {
        string key = p.first, value = p.second;
        if (key.length() >= 5 && key.substr(key.length()-5) == "color") {
            int r, g, b;
            if (sscanf(value.c_str(), "%d,%d,%d", &r, &g, &b) != 3) {
                cout << "WARNING: sscanf didnt return 3 here - value=" << value << "\n";
                continue;
            }
            kccolor color = {r%256, g%256, b%256};
            if (key == "bgcolor") config->bgcolor = color;
            if (key == "txtcolor") config->txtcolor = color;
            if (key == "graytxt") config->graytxt = color;
            if (key == "lightgraytxt") config->lightgraytxt = color;
        }
        else {
            if (key == "font") config->font = value;
            if (key == "fontsize") config->fontsize = stof(value);
            if (key == "scrollsensitivity") config->scrollsensitivity = stof(value);
            if (key == "width") config->width = stoi(value);
            if (key == "height") config->height = stoi(value);
        }
    }
    return true;
}
