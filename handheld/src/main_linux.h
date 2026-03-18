#ifndef MAIN_LINUX_H__
#define MAIN_LINUX_H__

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>

#include <sys/stat.h>
#include <unistd.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

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

#ifdef __EMSCRIPTEN__
EM_JS(void, web_setup_canvas_and_logs, (), {
    if (typeof Module !== 'undefined') {
        Module.print = function(text) { console.log(text); };
        Module.printErr = function(text) { console.error(text); };
    }

    if (typeof document === 'undefined') {
        return;
    }

    var body = document.body;
    if (!body) {
        return;
    }

    body.style.margin = '0';
    body.style.background = '#000';
    body.style.overflow = 'hidden';

    var canvas = Module.canvas || document.querySelector('canvas');
    if (!canvas) {
        return;
    }

    canvas.style.display = 'block';
    canvas.style.width = '100vw';
    canvas.style.height = '100vh';
    canvas.style.outline = 'none';
});

EM_JS(void, webfs_init, (), {
    if (typeof FS === 'undefined' || typeof IDBFS === 'undefined') {
        return;
    }

    if (!Module.__mcStorageState) {
        Module.__mcStorageState = {
            mounted: false,
            ready: false,
            syncing: false,
            lastError: 0
        };
    }

    var state = Module.__mcStorageState;
    try {
        FS.mkdir('/persistent');
    } catch (e) {}

    if (!state.mounted) {
        FS.mount(IDBFS, {}, '/persistent');
        state.mounted = true;
    }

    state.syncing = true;
    state.ready = false;
    FS.syncfs(true, function(err) {
        state.syncing = false;
        state.lastError = err ? 1 : 0;
        state.ready = true;
    });

    var flush = function() {
        if (!state.mounted || state.syncing) {
            return;
        }
        state.syncing = true;
        FS.syncfs(false, function(err) {
            state.syncing = false;
            state.lastError = err ? 1 : 0;
        });
    };

    if (!Module.__mcStorageHooksInstalled) {
        Module.__mcStorageHooksInstalled = true;
        if (typeof document !== 'undefined') {
            document.addEventListener('visibilitychange', function() {
                if (document.visibilityState === 'hidden') {
                    flush();
                }
            });
        }
        if (typeof window !== 'undefined') {
            window.addEventListener('beforeunload', flush);
            window.addEventListener('pagehide', flush);
        }
    }
});

EM_JS(int, webfs_ready, (), {
    return Module.__mcStorageState && Module.__mcStorageState.ready ? 1 : 0;
});

EM_JS(void, webfs_flush, (), {
    if (!Module.__mcStorageState) {
        return;
    }

    var state = Module.__mcStorageState;
    if (!state.mounted || state.syncing) {
        return;
    }

    state.syncing = true;
    FS.syncfs(false, function(err) {
        state.syncing = false;
        state.lastError = err ? 1 : 0;
    });
});
#endif

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
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) return 13;
    if (key == SDLK_ESCAPE) return Keyboard::KEY_ESCAPE;
    if (key == SDLK_TAB) return 250;
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
            if (event.key.keysym.sym >= 32 && event.key.keysym.sym <= 126) {
                Keyboard::feedText(event.key.keysym.sym);
            }
        }

        if (event.type == SDL_KEYUP) {
            unsigned char transformed = transformKey(event.key.keysym.sym);
            if (transformed) Keyboard::feed(transformed, 0);
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
#ifdef __EMSCRIPTEN__
    web_setup_canvas_and_logs();
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::printf("Couldn't initialize SDL2: %s\n", SDL_GetError());
        return -1;
    }

    int width = 854;
    int height = 480;
    AppPlatform_linux_setScreenSize(width, height);

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    g_window = SDL_CreateWindow(
        "Cobblestonium",
#ifdef __EMSCRIPTEN__
        0,
        0,
#else
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
#endif
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
#ifdef __EMSCRIPTEN__
    std::string storagePath = "/persistent";
#else
    std::string storagePath = getenv("HOME") ? getenv("HOME") : ".";
    storagePath += "/.minecraft/";
#endif
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

#ifdef __EMSCRIPTEN__
    emscripten_request_fullscreen("#canvas", EM_FALSE);

    double cssW = 0.0;
    double cssH = 0.0;
    if (emscripten_get_element_css_size("#canvas", &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS && cssW > 0.0 && cssH > 0.0) {
        width = (int)cssW;
        height = (int)cssH;
        SDL_SetWindowSize(g_window, width, height);
        AppPlatform_linux_setScreenSize(width, height);
    }

    webfs_init();

    struct MainLoopState {
        MAIN_CLASS* app;
        AppContext context;
        int width;
        int height;
        bool running;
        bool inited;
        int flushCounter;
    };

    MainLoopState* state = new MainLoopState();
    state->app = app;
    state->context = context;
    state->width = width;
    state->height = height;
    state->running = true;
    state->inited = false;
    state->flushCounter = 0;

    auto mainLoop = [](void* userData) {
        MainLoopState* s = (MainLoopState*)userData;

        if (!s->running || ((App*)s->app)->wantToQuit()) {
            webfs_flush();
            emscripten_cancel_main_loop();
            return;
        }

        if (!s->inited) {
            if (!webfs_ready()) {
                return;
            }

            ((App*)s->app)->init(s->context);
            ((App*)s->app)->setSize(s->width, s->height);
            s->inited = true;
            return;
        }

        int w = 0;
        int h = 0;
        SDL_GetWindowSize(g_window, &w, &h);
        if (w > 0 && h > 0) {
            AppPlatform_linux_setScreenSize(w, h);
            ((App*)s->app)->setSize(w, h);
        }

        s->running = handleEvents() == 0;
        ((App*)s->app)->update();
        SDL_GL_SwapWindow(g_window);

        if (++s->flushCounter >= 300) {
            webfs_flush();
            s->flushCounter = 0;
        }
    };

    emscripten_set_main_loop_arg(mainLoop, state, 0, 1);
#else
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
#endif

    delete app;
    SDL_GL_DeleteContext(g_glContext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}

#endif
