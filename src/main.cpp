#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <nfd.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include "config.hpp"

#define F(p,v)((p&v)==v)
#define KCVERSION "1.0-alpha1"

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
bool changes = false;

kcconfig userconfig;

string filepickerpath = "untitled";

enum kcerrs { Success, SDLInitFail, WindowNull, RendererNull, MainFontNull, TTFInitFail, HeaderFontNull };
enum kcscene { titlemenu, editor, openfl, savefl, exitscene };
kcscene scene, prevscene;

vector<pair<string, kcscene>> titlemenucmds = {
	make_pair("New...", editor),
	make_pair("Open...", openfl),
	make_pair("Exit", exitscene)
};
vector<pair<TTF_Text *, SDL_Rect>> commandrenders = {};

string titlestr = "KitCode v" + string(KCVERSION);

void resetTitle(bool showPath) {
	if (showPath) {
		string newtitle = titlestr + " | " + (changes ? "*" : "") + filepickerpath;
		SDL_SetWindowTitle(window, newtitle.c_str());
	}
	else SDL_SetWindowTitle(window, titlestr.c_str());
}

void switchScene(kcscene newscene) {
	if (newscene == editor) {
		resetTitle(true);
		SDL_StartTextInput(window);
	}
	else
		SDL_StopTextInput(window);

	if (newscene == titlemenu)
		resetTitle(false);
	
	prevscene = scene;
	scene = newscene;
}

int errS(int n, const char *s) {
	cout << "ERROR: " << s << "\n";
	return n;
}
int err(int n) {
	return errS(n, SDL_GetError());
}

bool areYouSure() {
	SDL_MessageBoxButtonData btns[] = {{
		.buttonID = 1,
		.text = "pretty sure"
	}, {
		.buttonID = 0,
		.text = "nah"
	}};
	SDL_MessageBoxData mbd = {
		.flags = SDL_MESSAGEBOX_WARNING,
		.window = window,
		.title = "question!",
		.message = "are you sure",
		.numbuttons = 2,
		.buttons = btns
	};
	int button;
	SDL_ShowMessageBox(&mbd, &button);
	return button;
}

bool handleGlobalShortcuts(SDL_Event e) {
	// OH MY GOD. so there was a bug where the shortcuts were handled twice for some reason
	// AND I JUST REALISED IT'S CUZ THE CTRL KEYS ARE PICKED UP HERE TOO
	// so when you press smth like ctrl+s and let go of ctrl first,
	// what this ends up doing first is skips the ctrl+s condition
	// AND GOES TO THE !changes || areYouSure() CONDITION
	// BUT BECAUSE IT'S NONE OF THOSE KEYS IT JUST WRITES THE S IN THE FRAME YOU LET GO OF THE S TOO
	// AND ONLY WHEN YOU PRESS CTRL+S, LETTING THE S GO FIRST, IT ACTUALLY PROCESSES CORRECTLY
	// BUT LUCKILY THIS LINE BELOW FIXES IT ALL. fml 
	if (e.key.key == SDLK_LCTRL || e.key.key == SDLK_RCTRL) return false;

	if (F(e.key.mod, SDL_KMOD_LCTRL) || F(e.key.mod, SDL_KMOD_RCTRL)) {
		changes = false;
		if (e.key.key == SDLK_S)
			switchScene(savefl);
		else if (!changes || areYouSure()) {
			if (e.key.key == SDLK_O)
				switchScene(openfl);
			else if (e.key.key == SDLK_N) {
				file.clear();
				cx = 0; cy = 0;
				filepickerpath = "untitled";
			}
		}
		else changes = true;
		return true;
	}
	return false;
}

void drawEditor() {
	bool processedshortcut = false;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_EVENT_QUIT) {
			if (!changes || areYouSure()) running = false;
		}
		else if (e.type == SDL_EVENT_KEY_DOWN) {
			changes = true;
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

			processedshortcut = handleGlobalShortcuts(e);
		}
		else if (e.type == SDL_EVENT_TEXT_INPUT && !processedshortcut) {
			file[cy] = file[cy].substr(0, cx) + e.text.text + file[cy].substr(cx, file[cy].length()-cx);
			cx++;
			changes = true;
			resetTitle(true);
		}
		else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
			scrollvalue += e.wheel.y * userconfig.scrollsensitivity;
		}
	}
	SDL_SetRenderDrawColor(renderer, UWKCC(userconfig.bgcolor), SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, NULL);

	// todo: don't render offscreen text
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

void filePickerLoadFile() {
	file.clear();
	ifstream f(filepickerpath);
	string line;
	while (!f.eof()) {
		getline(f, line);
		file.push_back(line);
	}
	cx = 0; cy = 0;
}
void filePickerStoreFile() {
	ofstream f(filepickerpath);
	for (int i = 0; i < file.size(); i++)
		f << file[i] << (i == file.size()-1 ? "" : "\n");
}

void filePickerOpen() {
	string pwd = filesystem::current_path().string();
	char *outflname = new char[512]; // ! possibly unsafe
	nfdresult_t result = NFD_OpenDialog("", pwd.c_str(), &outflname);
	if (result == NFD_OKAY) {
		filepickerpath = outflname;
		filePickerLoadFile();
		switchScene(editor);
	}
	else switchScene(prevscene);
	delete outflname;
	// filePickerBase("Open...", false);
}
void filePickerSave() {
	string pwd = filesystem::current_path().string();
	char *outflname = new char[512]; // ! possibly unsafe
	nfdresult_t result = NFD_SaveDialog("", pwd.c_str(), &outflname);
	if (result == NFD_OKAY) {
		filepickerpath = outflname;
		filePickerStoreFile();
		switchScene(editor);
	}
	else switchScene(prevscene);
	delete outflname;
	// filePickerBase("Save...", true);
}

void drawTitleMenu() {
	if (commandrenders.size() == 0) {
		int yoff = 0;
		for (auto &cmd : titlemenucmds) {
			TTF_Text *text = TTF_CreateText(textengine, font, cmd.first.c_str(), cmd.first.length());
			SDL_Rect rect{};
			TTF_GetTextSize(text, &rect.w, &rect.h);
			rect.x = (winwidth-rect.w)/2;
			rect.y = (winwidth-rect.h)/3 + yoff;
			yoff += rect.h;
			commandrenders.push_back(make_pair(text, rect));
		}
	}

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_EVENT_QUIT) running = false;
		else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
			SDL_Point mouse = {e.button.x, e.button.y};
			for (int i = 0; i < commandrenders.size(); i++) {
				if (SDL_PointInRect(&mouse, &commandrenders[i].second))
					switchScene(titlemenucmds[i].second);
			}
		}
	}
	SDL_SetRenderDrawColor(renderer, UWKCC(userconfig.bgcolor), SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, NULL);

	// todo: add recent files
	// todo unrelated to this function: add plugins in lua

	TTF_Text *header = TTF_CreateText(textengine, headerfont, titlestr.c_str(), titlestr.length());
	int headerw, headerh;
	TTF_GetTextSize(header, &headerw, &headerh);
	TTF_DrawRendererText(header, (winwidth-headerw)/2, (winheight-headerh)/4);
	TTF_DestroyText(header);

	for (auto &p : commandrenders)
		TTF_DrawRendererText(p.first, p.second.x, p.second.y);
}

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

	switchScene(titlemenu);

	while (running) {
		SDL_RenderPresent(renderer);
		if (scene == editor) drawEditor();
		// note: both filePickerSave and filePickerOpen use nativefiledialog instead of a custom-made file dialog to simplify shit
		//       and once a file is selected the scene switches to editor
		else if (scene == savefl) filePickerSave();
		else if (scene == openfl) filePickerOpen();
		else if (scene == titlemenu) drawTitleMenu();
		else if (scene == exitscene) running = false;
		SDL_Delay(20); // 50fps capped. fuck you why would you need more
	}

	for (auto &r : commandrenders) TTF_DestroyText(r.first);
	commandrenders.clear();

	TTF_DestroyRendererTextEngine(textengine);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
