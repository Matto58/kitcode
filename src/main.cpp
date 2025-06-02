#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include "config.hpp"

using namespace std;

int winwidth = 1600, winheight = 900;
size_t cx = 0, cy = 0;
vector<string> file{""};
int scrollvalue = 0;

SDL_Window *window;
SDL_Renderer *renderer;
TTF_TextEngine *textengine;
TTF_Font *font, *headerfont;
SDL_Event e;
bool running = true;

kcconfig userconfig;

filesystem::path filepickerdir;
struct filepickerentry {
	bool isDir;
	filesystem::path path;
	string customDisplay = "";
	bool useCustomDisplay = false;
};
vector<filepickerentry> filepickerlisting{};
string filepickerselection;

void navigateFilePicker() {
	//cout << "navigateFilePicker\n";
	filepickerlisting.clear();
	filepickerlisting.push_back({true, filepickerdir.parent_path(), "..", true});
	for (auto &entry : filesystem::directory_iterator(filepickerdir)) {
		//cout << entry.path() << "\n";
		filepickerlisting.push_back({entry.is_directory(), entry.path()});
	}
	//sort(filepickerlisting.begin(), filepickerlisting.end());
}

enum kcscene { titlemenu, editor, openfl, savefl };
kcscene scene;
void switchScene(kcscene newscene) {
	if (newscene == editor || newscene == savefl)
		SDL_StartTextInput(window);
	else
		SDL_StopTextInput(window);

	if (newscene == openfl || newscene == savefl) {
		filepickerdir = filesystem::current_path();
		navigateFilePicker();
	}
	
	scene = newscene;
}

int errS(int n, const char *s) {
	cout << "ERROR: " << s << "\n";
	return n;
}
int err(int n) {
	return errS(n, SDL_GetError());
}

void drawEditor() {
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_EVENT_QUIT) running = false;
		else if (e.type == SDL_EVENT_TEXT_INPUT) {
			file[cy] = file[cy].substr(0, cx) + e.text.text + file[cy].substr(cx, file[cy].length()-cx);
			cx++;
		}
		else if (e.type == SDL_EVENT_KEY_DOWN) {
			if (e.key.key == SDLK_BACKSPACE && !(cy == 0 && cx == 0)) {
				if (cx > 0) {
					file[cy] = file[cy].substr(0, cx-1) + file[cy].substr(cx);
					cx--;
				}
				else {
					cx = file[cy-1].length();
					file[cy-1] += file[cy];
					file.erase(file.begin() + cy);
					cy--;
				}
			}
			else if (e.key.key == SDLK_UP && cy > 0) {
				cy--;
				cx = min(file[cy].length(), cx);
			}
			else if (e.key.key == SDLK_DOWN && cy < file.size()-1) {
				cy++;
				cx = min(file[cy].length(), cx);
			}
			// fuck you, vim-style left/right
			else if (e.key.key == SDLK_LEFT) {
				if (cx > 0) cx--;
			}
			else if (e.key.key == SDLK_RIGHT) {
				if (cx < file[cy].length()) cx++;
			}
			else if (e.key.key == SDLK_RETURN) {
				cy++;
				file.insert(file.begin() + cy, file[cy-1].length() == 0 ? "" : file[cy-1].substr(cx));
				file[cy-1] = file[cy-1].substr(0, cx);
				cx = 0;
			}
			else if (e.key.key == SDLK_DELETE) {
				if (cx == file[cy].length()) {
					if (cy+1 != file.size()) {
						file[cy] += file[cy+1];
						file.erase(file.begin() + cy + 1);
					}
				}
				else {
					file[cy] = file[cy].substr(0, cx) + file[cy].substr(cx+1);
				}
			}
		}
		else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
			scrollvalue += e.wheel.y * userconfig.scrollsensitivity;
		}
	}
	SDL_SetRenderDrawColor(renderer, UWKCC(userconfig.bgcolor), SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, NULL);

	int fheight = TTF_GetFontHeight(font);
	TTF_Text *line, *linenum;
	size_t lineID = 0;
	for (string linestr : file) {
		string lineIDstr = to_string(lineID+1);
		linenum = TTF_CreateText(textengine, font, lineIDstr.c_str(), lineIDstr.length());
		if (lineID == cy) TTF_SetTextColor(linenum, UWKCC(userconfig.lightgraytxt), SDL_ALPHA_OPAQUE);
		else TTF_SetTextColor(linenum, UWKCC(userconfig.graytxt), SDL_ALPHA_OPAQUE);
		TTF_DrawRendererText(linenum, 4, fheight*lineID + 4 + scrollvalue);
		TTF_DestroyText(linenum);

		line = TTF_CreateText(textengine, font, linestr.c_str(), linestr.length());
		TTF_SetTextColor(line, UWKCC(userconfig.txtcolor), SDL_ALPHA_OPAQUE);
		TTF_DrawRendererText(line, 64, fheight*lineID + 4 + scrollvalue);
		TTF_DestroyText(line);

		lineID++;
	}
	// motherfucking hack
	string cursorstring = "_";
	TTF_Text *cursor = TTF_CreateText(textengine, font, cursorstring.c_str(), cursorstring.length());
	TTF_Text *clippedLine = TTF_CreateText(textengine, font, file[cy].substr(0, cx).c_str(), cx);
	int cxrendered;
	TTF_GetTextSize(clippedLine, &cxrendered, NULL);
	TTF_SetTextColor(cursor, UWKCC(userconfig.lightgraytxt), SDL_ALPHA_OPAQUE);
	TTF_DrawRendererText(cursor, 64 + cxrendered, fheight*cy + 4 + scrollvalue);
	TTF_DestroyText(cursor);
	TTF_DestroyText(clippedLine);
}

// todo: move all of this and other file picker related functions into filepicker.cpp
void filePickerBase(string actionName, bool showFlnameInput) {
	SDL_Point cliccykitty = {-1, -1};
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_EVENT_QUIT) running = false;
		else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
			cliccykitty.x = e.button.x;
			cliccykitty.y = e.button.y;
			cout << "DEBUG: mouse button up event @ " << to_string(cliccykitty.x) << ", " << to_string(cliccykitty.y) << "\n";
		}
	}

	SDL_SetRenderDrawColor(renderer, UWKCC(userconfig.bgcolor), SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, NULL);

	string headerstr = actionName + " | " + filepickerdir.generic_string();
	TTF_Text *header = TTF_CreateText(textengine, headerfont, headerstr.c_str(), headerstr.length());
	TTF_SetTextColor(header, UWKCC(userconfig.txtcolor), SDL_ALPHA_OPAQUE);
	int listingmargin;
	TTF_GetTextSize(header, NULL, &listingmargin);
	listingmargin += 6;
	TTF_DrawRendererText(header, 4, 4);
	TTF_DestroyText(header);

	TTF_Text *pathname;
	int currentheight = 0;

	filesystem::path newpath;
	bool newpathbool = false;

	for (auto &entry : filepickerlisting) {
		string displayedpath = entry.path.filename().string();
		if (entry.isDir) displayedpath += "/";

		SDL_Rect pathrect{};
		pathrect.x = 4;
		pathrect.y = listingmargin + currentheight;
		if (entry.useCustomDisplay)
			pathname = TTF_CreateText(textengine, font, entry.customDisplay.c_str(), entry.customDisplay.length());
		else
			pathname = TTF_CreateText(textengine, font, displayedpath.c_str(), displayedpath.length());
		TTF_SetTextColor(pathname, UWKCC(userconfig.txtcolor), SDL_ALPHA_OPAQUE);
		TTF_DrawRendererText(pathname, pathrect.x, pathrect.y);
		TTF_GetTextSize(pathname, &pathrect.w, &pathrect.h);
		currentheight += pathrect.h;

		if (SDL_PointInRect(&cliccykitty, &pathrect)) {
			newpath = filepickerdir / entry.path;
			//cout << entry.isDir << "\n" << entry.path << "\n";
			if (!entry.isDir) {
				file.clear();
				ifstream f(newpath);
				string line;
				while (!f.eof()) {
					getline(f, line);
					file.push_back(line);
				}
				cx = 0; cy = 0;
				switchScene(editor);
			}
			else newpathbool = true;
		}

		TTF_DestroyText(pathname);
	}

	filepickerdir = newpath;
	if (newpathbool) navigateFilePicker();
}

void filePickerOpen() {
	filePickerBase("Open...", false);
}
void filePickerSave() {
	filePickerBase("Save...", true);
}

enum kcerrs {
	Success, SDLInitFail, WindowNull, RendererNull, MainFontNull, TTFInitFail, HeaderFontNull 
};

int main(int argc, char **argv) {
	userconfig = {};
	if (!loadConfig("config.ini", &userconfig))
		cout << "WARNING: could not load config.ini, using default config\n";

	if (!SDL_Init(SDL_INIT_VIDEO)) return err(SDLInitFail);
	if (!TTF_Init()) return err(TTFInitFail);
	window = SDL_CreateWindow("KitCode", winwidth, winheight, SDL_WINDOW_INPUT_FOCUS);
	if (window == NULL) return err(WindowNull);
	renderer = SDL_CreateRenderer(window, NULL);
	if (renderer == NULL) {
		SDL_DestroyWindow(window);
		return err(RendererNull);
	}

	textengine = TTF_CreateRendererTextEngine(renderer);
	font = TTF_OpenFont(userconfig.font.c_str(), userconfig.fontsize);
	if (font == NULL) {
		TTF_DestroyRendererTextEngine(textengine);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		return err(MainFontNull);
	}
	headerfont = TTF_OpenFont(userconfig.headerfont.c_str(), userconfig.headerfontsize);
	if (headerfont == NULL) {
		TTF_DestroyRendererTextEngine(textengine);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		return err(HeaderFontNull);
	}

	filepickerlisting.reserve(10);
	switchScene(openfl);

	while (running) {
		SDL_RenderPresent(renderer);
		if (scene == editor) drawEditor();
		else if (scene == savefl) filePickerSave();
		else if (scene == openfl) filePickerOpen();
		SDL_Delay(20); // 50fps capped. fuck you why would you need more
	}

	TTF_DestroyRendererTextEngine(textengine);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
