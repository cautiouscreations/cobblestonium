#ifndef MAIN_LINUX_H__
#define MAIN_LINUX_H__

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>

#include <sys/stat.h>
#include <unistd.h>

#include "App.h"
#include "AppPlatform_linux.h"
#include "NinecraftApp.h"
#include "client/renderer/gles.h"
#include "platform/input/Keyboard.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"

#ifndef MAIN_CLASS
#define MAIN_CLASS NinecraftApp
#endif

void AppPlatform_linux_setScreenSize(int width, int height);

static SDL_Window* g_window = NULL;
static SDL_GLContext g_glContext = NULL;
static bool g_app_window_normal = true;

static bool fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

static std::string parentDir(const std::string& path) {
    size_t pos = path.rfind('/');
    if (pos == std::string::npos) {
        return ".";
    }
    if (pos == 0) {
        return "/";
    }
    return path.substr(0, pos);
}

static void switchToAssetRoot(const char* argv0) {
    char cwdBuf[4096];
    if (getcwd(cwdBuf, sizeof(cwdBuf))) {
        std::string cwd = cwdBuf;
        if (fileExists(cwd + "/data/images/terrain.png")) {
            return;
        }
    }

    std::string exePath = argv0 ? argv0 : "";
    if (!exePath.empty() && exePath[0] != '/') {
        char absBuf[4096];
        if (realpath(exePath.c_str(), absBuf)) {
            exePath = absBuf;
        }
    }

    std::string exeDir = parentDir(exePath);
    std::string candidates[4] = {
        exeDir,
        exeDir + "/..",
        exeDir + "/../..",
        exeDir + "/../../handheld"
    };

    for (int i = 0; i < 4; ++i) {
        const std::string& dir = candidates[i];
        if (fileExists(dir + "/data/images/terrain.png")) {
            chdir(dir.c_str());
            return;
        }
        if (fileExists(dir + "/handheld/data/images/terrain.png")) {
            std::string handheldDir = dir + "/handheld";
            chdir(handheldDir.c_str());
            return;
        }
    }
}

static unsigned char transformKey(int key) {
    if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) return Keyboard::KEY_LSHIFT;
    if (key == SDLK_DOWN) return 40;
    if (key == SDLK_UP) return 38;
    if (key == SDLK_SPACE) return Keyboard::KEY_SPACE;
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) return Keyboard::KEY_RETURN;
    if (key == SDLK_ESCAPE) return Keyboard::KEY_ESCAPE;
    if (key == SDLK_TAB) return 250;
    if (key == SDLK_BACKSPACE) return Keyboard::KEY_BACKSPACE;
    if (key >= SDLK_a && key <= SDLK_z) return (unsigned char)('A' + (key - SDLK_a));
    if (key >= SDLK_0 && key <= SDLK_9) return (unsigned char)('0' + (key - SDLK_0));
    return 0;
}

static int handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return -1;
        }

        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                g_app_window_normal = true;
            } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST || event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                g_app_window_normal = false;
            } else if (event.window.event == SDL_WINDOWEVENT_RESTORED || event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
                g_app_window_normal = true;
            }
        }

        if (event.type == SDL_KEYDOWN) {
            unsigned char transformed = transformKey(event.key.keysym.sym);
            if (transformed) Keyboard::feed(transformed, 1);
        }

        if (event.type == SDL_KEYUP) {
            unsigned char transformed = transformKey(event.key.keysym.sym);
            if (transformed) Keyboard::feed(transformed, 0);
        }

        if (event.type == SDL_TEXTINPUT) {
            for (int i = 0; event.text.text[i] != '\0'; i++) {
                Keyboard::feedText(event.text.text[i]);
            }
        }

        if (event.type == SDL_MOUSEWHEEL) {
            int mx = 0;
            int my = 0;
            SDL_GetMouseState(&mx, &my);
            if (event.wheel.y > 0) {
                Mouse::feed(3, 0, mx, my, 0, 1);
            } else if (event.wheel.y < 0) {
                Mouse::feed(3, 0, mx, my, 0, -1);
            }
        }

        if (event.type == SDL_MOUSEBUTTONDOWN) {
            bool left = event.button.button == SDL_BUTTON_LEFT;
            char button = left ? 1 : 2;
            Mouse::feed(button, 1, event.button.x, event.button.y);
            Multitouch::feed(button, 1, event.button.x, event.button.y, 0);
        }

        if (event.type == SDL_MOUSEBUTTONUP) {
            bool left = event.button.button == SDL_BUTTON_LEFT;
            char button = left ? 1 : 2;
            Mouse::feed(button, 0, event.button.x, event.button.y);
            Multitouch::feed(button, 0, event.button.x, event.button.y, 0);
        }

        if (event.type == SDL_MOUSEMOTION) {
            float x = (float)event.motion.x;
            float y = (float)event.motion.y;
            Multitouch::feed(0, 0, x, y, 0);
            Mouse::feed(0, 0, x, y, event.motion.xrel, event.motion.yrel);
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::printf("Couldn't initialize SDL2: %s\n", SDL_GetError());
        return -1;
    }

    int width = 854;
    int height = 480;
    AppPlatform_linux_setScreenSize(width, height);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    g_window = SDL_CreateWindow(
        "Cobblestonium",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!g_window) {
        std::printf("Couldn't create SDL2 window: %s\n", SDL_GetError());
        SDL_Quit();
        return -2;
    }

    g_glContext = SDL_GL_CreateContext(g_window);
    if (!g_glContext) {
        std::printf("Couldn't create GL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return -3;
    }

    glInit();

    switchToAssetRoot(argv[0]);

    MAIN_CLASS* app = new MAIN_CLASS();
    std::string storagePath = getenv("HOME") ? getenv("HOME") : ".";
    storagePath += "/.minecraft/";
    app->externalStoragePath = storagePath;
    app->externalCacheStoragePath = storagePath;

    int commandPort = 0;
    if (argc > 1) {
        commandPort = std::atoi(argv[1]);
    }
    if (commandPort != 0) {
        app->commandPort = commandPort;
    }

    AppContext context;
    AppPlatform_linux platform;
    context.doRender = false;
    context.platform = &platform;

    ((App*)app)->init(context);
    ((App*)app)->setSize(width, height);

    bool running = true;
    while (running && !((App*)app)->wantToQuit()) {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(g_window, &w, &h);
        if (w > 0 && h > 0) {
            AppPlatform_linux_setScreenSize(w, h);
            ((App*)app)->setSize(w, h);
        }

        running = handleEvents() == 0;
        ((App*)app)->update();
        if (g_app_window_normal) {
            SDL_GL_SwapWindow(g_window);
        } else {
            SDL_Delay(16);
        }
    }

    delete app;
    SDL_GL_DeleteContext(g_glContext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}

#endif
