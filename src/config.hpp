#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <fstream>
#include <string>
using namespace std;

// UnWrap KitCode Color
#define UWKCC(c) c.r, c.g, c.b

struct kccolor { unsigned char r, g, b; };
struct kcconfig {
    string font = "firamono.ttf", headerfont = "roboto.ttf";
    float fontsize = 16, headerfontsize = 28;
    kccolor bgcolor = {15, 15, 31};
    kccolor txtcolor = {255, 255, 255};
    kccolor graytxt = {127, 127, 127};
    kccolor lightgraytxt = {191, 191, 191};
    float scrollsensitivity = 36;
};

bool loadConfig(string filename, kcconfig *config);

#endif