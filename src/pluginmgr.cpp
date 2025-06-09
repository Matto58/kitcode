#include <fstream>
#include "pluginmgr.hpp"

bool loadPluginInfo(string pluginid, kcplugin *plugin) {
    if (plugin == NULL) return false;
    plugin->id = pluginid;
    ifstream file("plugins/" + pluginid + "/main.lua");
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
        if (key == "NAME") plugin->name = value;
        else if (key == "VERSION") plugin->version = value;
        else if (key == "AUTHOR") plugin->author = value;
    }
    file.close();
    return true;
}

vector<kcplugin> loadPlugins(string includepath) {
    ifstream f(includepath);
    vector<kcplugin> v{};
    string word;
    while (!f.eof()) {
        f >> word;
        kcplugin plugin{};
        if (loadPluginInfo(word, &plugin)) v.push_back(plugin);
    }
    f.close();
    return v;
}
