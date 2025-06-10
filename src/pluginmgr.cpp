#include "pluginmgr.hpp"
#include "parseini.hpp"

bool loadPluginInfo(string pluginid, kcplugin *plugin) {
    if (plugin == NULL) return false;
    plugin->id = pluginid;
    map<string, string> m = parseIni("plugins/" + pluginid + "/info.ini");
    for (auto &p : m) {
        if (p.first == "NAME") plugin->name = p.second;
        else if (p.first == "VERSION") plugin->version = p.second;
        else if (p.first == "AUTHOR") plugin->author = p.second;
    }
    return true;
}

vector<kcplugin> loadPlugins(string includepath) {
    ifstream f(includepath);
    vector<kcplugin> v{};
    string word;
    while (!f.eof()) {
        f >> word;
        kcplugin plugin{};
        loadPluginInfo(word, &plugin);
        v.push_back(plugin);
    }
    f.close();
    return v;
}
