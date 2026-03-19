#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__CreateWorldScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__CreateWorldScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "../components/TextBox.h"

class CreateWorldScreen : public Screen {
public:
    CreateWorldScreen();
    virtual ~CreateWorldScreen();

    virtual void init();
    virtual void setupPositions();
    virtual void render(int xm, int ym, float a);
    virtual void buttonClicked(Button* button);
    virtual bool handleBackEvent(bool isDown);
    virtual void tick();

private:
    TextBox* nameBox;
    TextBox* seedBox;
    Button* bStart;
    Button* bGameMode;
    Button* bBack;

    bool isCreative;
};

#endif
