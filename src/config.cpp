#include "config.hpp"

bool loadConfig(string filename, kcconfig *config) {
    if (config == NULL) return false;
    ifstream file(filename);
    string line;
    while (!file.eof()) {
        getline(file, line);
        string key, value;
        for (int i = 0; i < line.length(); i++) {
            if (line[i] == '=') {
                value = line.substr(i+1);
                break;
            }
            else key += line[i];
        }
        if (key.length() >= 5 && key.substr(key.length()-5) == "color") {
            int r, g, b;
            sscanf(value.c_str(), "%d,%d,%d", &r, &g, &b);
            kccolor color = {r, g, b}; // can have unintended results when r, g or b aren't within 0-255
            if (key == "bgcolor") config->bgcolor = color;
            if (key == "txtcolor") config->txtcolor = color;
            if (key == "graytxt") config->graytxt = color;
            if (key == "lightgraytxt") config->lightgraytxt = color;
        }
        else {
            if (key == "font") config->font = value;
            if (key == "fontsize") config->fontsize = stof(value);
            if (key == "scrollsensitivity") config->scrollsensitivity = stof(value);
        }
    }
    file.close();
    return true;
}
