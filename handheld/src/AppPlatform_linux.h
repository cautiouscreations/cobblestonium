#ifndef APPPLATFORM_LINUX_H__
#define APPPLATFORM_LINUX_H__

#include "AppPlatform.h"
#include "platform/log.h"
#include "world/level/storage/FolderMethods.h"

#include <png.h>

#include <fstream>
#include <sstream>

static void png_funcReadFile_linux(png_structp pngPtr, png_bytep data, png_size_t length) {
    ((std::istream*)png_get_io_ptr(pngPtr))->read((char*)data, length);
}

class AppPlatform_linux: public AppPlatform {
public:
    BinaryBlob readAssetFile(const std::string& filename) {
        FILE* fp = fopen(("data/" + filename).c_str(), "r");
        if (!fp)
            return BinaryBlob();

        int size = getRemainingFileSize(fp);

        BinaryBlob blob;
        blob.size = size;
        blob.data = new unsigned char[size];

        fread(blob.data, 1, size, fp);
        fclose(fp);

        return blob;
    }

    TextureData loadTexture(const std::string& filename_, bool textureFolder) {
        TextureData out;

        std::string filename = textureFolder ? "data/images/" + filename_ : filename_;
        std::ifstream source(filename.c_str(), std::ios::binary);

        if (!source) {
            LOGI("Couldn't find file: %s\n", filename.c_str());
            return out;
        }

        png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!pngPtr)
            return out;

        png_infop infoPtr = png_create_info_struct(pngPtr);
        if (!infoPtr) {
            png_destroy_read_struct(&pngPtr, NULL, NULL);
            return out;
        }

        png_set_read_fn(pngPtr, (png_voidp)&source, png_funcReadFile_linux);
        png_read_info(pngPtr, infoPtr);

        out.w = png_get_image_width(pngPtr, infoPtr);
        out.h = png_get_image_height(pngPtr, infoPtr);

        png_bytep* rowPtrs = new png_bytep[out.h];
        out.data = new unsigned char[4 * out.w * out.h];
        out.memoryHandledExternally = false;

        int rowStrideBytes = 4 * out.w;
        for (int i = 0; i < out.h; i++) {
            rowPtrs[i] = (png_bytep)&out.data[i * rowStrideBytes];
        }
        png_read_image(pngPtr, rowPtrs);

        png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
        delete[] (png_bytep)rowPtrs;

        return out;
    }

    std::string getDateString(int s) {
        std::stringstream ss;
        ss << s << " s (UTC)";
        return ss.str();
    }

    int checkLicense() {
        return 0;
    }

    int getScreenWidth();
    int getScreenHeight();
    float getPixelsPerMillimeter();
    bool supportsTouchscreen();
    bool hasBuyButtonWhenInvalidLicense();
    void showKeyboard();
    void hideKeyboard();
};

#endif
