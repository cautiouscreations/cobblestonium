#ifndef MAIN_WEB_H__
#define MAIN_WEB_H__

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>

#include <emscripten.h>
#include <emscripten/html5.h>

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

struct QueuedTouchEvent {
    char type;
    short x;
    short y;
    char pointerId;
};


static long g_activeTouchIds[12] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

static char getInternalPointerId(long identifier, bool allocate) {
    for (int i = 0; i < 12; ++i) {
        if (g_activeTouchIds[i] == identifier) {
            return (char)i;
        }
    }
    if (allocate) {
        for (int i = 0; i < 12; ++i) {
            if (g_activeTouchIds[i] == -1) {
                g_activeTouchIds[i] = identifier;
                return (char)i;
            }
        }
        return 11;
    }
    return -1;
}

static void releaseInternalPointerId(long identifier) {
    for (int i = 0; i < 12; ++i) {
        if (g_activeTouchIds[i] == identifier) {
            g_activeTouchIds[i] = -1;
            break;
        }
    }
}

static QueuedTouchEvent g_touchQueue[512];
static int g_touchQueueCount = 0;
static char g_primaryPointerId = -1;

static void queueTouchEvent(char type, short x, short y, char pointerId) {
    if (g_touchQueueCount < 512) {
        g_touchQueue[g_touchQueueCount++] = {type, x, y, pointerId};
    }
}

static void processQueuedTouchEvents() {
    for (int i = 0; i < g_touchQueueCount; ++i) {
        QueuedTouchEvent& e = g_touchQueue[i];
        bool isPrimary = (e.pointerId == g_primaryPointerId);
        if (e.type == 1) {
            if (isPrimary) {
                Mouse::feed(1, 1, e.x, e.y);
            }
            Multitouch::feed(1, 1, e.x, e.y, e.pointerId);
        } else if (e.type == 0) {
            if (isPrimary) {
                Mouse::feed(1, 0, e.x, e.y);
                g_primaryPointerId = -1;
            }
            Multitouch::feed(1, 0, e.x, e.y, e.pointerId);
        } else if (e.type == 2) {
            if (isPrimary) {
                Mouse::feed(0, 0, e.x, e.y, 0, 0);
            }
            Multitouch::feed(0, 0, e.x, e.y, e.pointerId);
        }
    }
    g_touchQueueCount = 0;
}

EM_BOOL web_touch_start(int eventType, const EmscriptenTouchEvent* e, void* userData) {
    (void)eventType;
    (void)userData;
    for (int i = 0; i < e->numTouches; ++i) {
        const EmscriptenTouchPoint* t = &e->touches[i];
        if (t->isChanged) {
            short x = (short)t->targetX;
            short y = (short)t->targetY;
            char pointerId = getInternalPointerId(t->identifier, true);
            if (g_primaryPointerId == -1) {
                g_primaryPointerId = pointerId;
            }
            queueTouchEvent(1, x, y, pointerId);
        }
    }
    return EM_TRUE;
}

EM_BOOL web_touch_end(int eventType, const EmscriptenTouchEvent* e, void* userData) {
    (void)eventType;
    (void)userData;
    for (int i = 0; i < e->numTouches; ++i) {
        const EmscriptenTouchPoint* t = &e->touches[i];
        if (t->isChanged) {
            short x = (short)t->targetX;
            short y = (short)t->targetY;
            char pointerId = getInternalPointerId(t->identifier, false);
            if (pointerId != -1) {
                queueTouchEvent(0, x, y, pointerId);
                releaseInternalPointerId(t->identifier);
            }
        }
    }
    return EM_TRUE;
}

EM_BOOL web_touch_move(int eventType, const EmscriptenTouchEvent* e, void* userData) {
    (void)eventType;
    (void)userData;
    for (int i = 0; i < e->numTouches; ++i) {
        const EmscriptenTouchPoint* t = &e->touches[i];
        if (t->isChanged) {
            short x = (short)t->targetX;
            short y = (short)t->targetY;
            char pointerId = getInternalPointerId(t->identifier, false);
            if (pointerId != -1) {
                queueTouchEvent(2, x, y, pointerId);
            }
        }
    }
    return EM_TRUE;
}

EM_BOOL web_touch_cancel(int eventType, const EmscriptenTouchEvent* e, void* userData) {
    return web_touch_end(eventType, e, userData);
}

static int handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return -1;
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
            if (event.wheel.which == SDL_TOUCH_MOUSEID) continue;
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
            if (event.button.which == SDL_TOUCH_MOUSEID) continue;
            bool left = event.button.button == SDL_BUTTON_LEFT;
            char button = left ? 1 : 2;
            Mouse::feed(button, 1, event.button.x, event.button.y);
            Multitouch::feed(button, 1, event.button.x, event.button.y, 0);
        }

        if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.which == SDL_TOUCH_MOUSEID) continue;
            bool left = event.button.button == SDL_BUTTON_LEFT;
            char button = left ? 1 : 2;
            Mouse::feed(button, 0, event.button.x, event.button.y);
            Multitouch::feed(button, 0, event.button.x, event.button.y, 0);
        }

        if (event.type == SDL_MOUSEMOTION) {
            if (event.motion.which == SDL_TOUCH_MOUSEID) continue;
            float x = (float)event.motion.x;
            float y = (float)event.motion.y;
            Multitouch::feed(0, 0, x, y, 0);
            Mouse::feed(0, 0, x, y, event.motion.xrel, event.motion.yrel);
        }
    }

    processQueuedTouchEvents();
    return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    web_setup_canvas_and_logs();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::printf("Couldn't initialize SDL2: %s\n", SDL_GetError());
        return -1;
    }

    int width = 854;
    int height = 480;
    AppPlatform_linux_setScreenSize(width, height);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    g_window = SDL_CreateWindow(
        "Cobblestonium",
        0,
        0,
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

    MAIN_CLASS* app = new MAIN_CLASS();
    std::string storagePath = "/persistent";
    app->externalStoragePath = storagePath;
    app->externalCacheStoragePath = storagePath;

    AppContext context;
    AppPlatform_linux platform;
    context.doRender = false;
    context.platform = &platform;

    emscripten_request_fullscreen("#canvas", EM_FALSE);

    emscripten_set_touchstart_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, web_touch_start);
    emscripten_set_touchend_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, web_touch_end);
    emscripten_set_touchmove_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, web_touch_move);
    emscripten_set_touchcancel_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, web_touch_cancel);

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
        if (w > 0 && h > 0 && (w != s->width || h != s->height)) {
            s->width = w;
            s->height = h;
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

    delete app;
    SDL_GL_DeleteContext(g_glContext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}

#endif
