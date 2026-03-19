#include "AppPlatform_linux.h"

#include "util/Mth.h"

static int g_screenWidth = 854;
static int g_screenHeight = 480;

void AppPlatform_linux_setScreenSize(int width, int height) {
    g_screenWidth = width;
    g_screenHeight = height;
}

int AppPlatform_linux::getScreenWidth() {
    return g_screenWidth;
}

int AppPlatform_linux::getScreenHeight() {
    return g_screenHeight;
}

float AppPlatform_linux::getPixelsPerMillimeter() {
    const int w = g_screenWidth;
    const int h = g_screenHeight;
    const float pixels = Mth::sqrt(w * w + h * h);
    const float mm = 7.0f * 25.4f;
    return pixels / mm;
}

bool AppPlatform_linux::supportsTouchscreen() {
#ifdef WEB
    return true;
#else
    return false;
#endif
}

bool AppPlatform_linux::hasBuyButtonWhenInvalidLicense() {
    return false;
}
