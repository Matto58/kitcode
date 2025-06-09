#include <string>
#include <vector>
#include <fstream>
using namespace std;

struct kcplugin { string id, name, version, author; };

bool loadPluginInfo(string pluginid, kcplugin *plugin);
vector<kcplugin> loadPlugins(string includepath);
