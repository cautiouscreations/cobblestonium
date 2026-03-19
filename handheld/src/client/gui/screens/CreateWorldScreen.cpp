#include "CreateWorldScreen.h"
#include "ProgressScreen.h"
#include "../../Minecraft.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../platform/time.h"
#include "../../../util/StringUtils.h"

CreateWorldScreen::CreateWorldScreen()
:   nameBox(NULL),
    seedBox(NULL),
    bStart(NULL),
    bGameMode(NULL),
    bBack(NULL),
    isCreative(false)
{
}

CreateWorldScreen::~CreateWorldScreen()
{
    delete nameBox;
    delete seedBox;
    delete bStart;
    delete bGameMode;
    delete bBack;
}

void CreateWorldScreen::init()
{
    nameBox = new TextBox(0, "New World");
    seedBox = new TextBox(1, "");
    bStart = new Button(2, "Start");
    bGameMode = new Button(3, "Game Mode: Survival");
    bBack = new Button(4, "Back");

    textBoxes.push_back(nameBox);
    textBoxes.push_back(seedBox);
    buttons.push_back(bStart);
    buttons.push_back(bGameMode);
    buttons.push_back(bBack);

    nameBox->setFocus(minecraft);
}

void CreateWorldScreen::setupPositions()
{
    nameBox->w = 200;
    nameBox->h = 24;
    nameBox->x = (width - nameBox->w) / 2;
    nameBox->y = 40;

    seedBox->w = 200;
    seedBox->h = 24;
    seedBox->x = (width - seedBox->w) / 2;
    seedBox->y = 80;

    bGameMode->width = 200;
    bGameMode->x = (width - bGameMode->width) / 2;
    bGameMode->y = 120;

    bStart->width = 100;
    bStart->x = width / 2 - 105;
    bStart->y = height - 40;

    bBack->width = 100;
    bBack->x = width / 2 + 5;
    bBack->y = height - 40;
}

void CreateWorldScreen::render(int xm, int ym, float a)
{
    renderBackground();
    drawCenteredString(font, "World Name", width / 2, 30, 0xffffffff);
    drawCenteredString(font, "Seed (Optional)", width / 2, 70, 0xffffffff);
    Screen::render(xm, ym, a);
}

void CreateWorldScreen::buttonClicked(Button* button)
{
    if (button == bBack) {
        minecraft->setScreen(NULL);
        return;
    }

    if (button == bGameMode) {
        isCreative = !isCreative;
        bGameMode->msg = isCreative ? "Game Mode: Creative" : "Game Mode: Survival";
        return;
    }

    if (button == bStart) {
        std::string levelName = nameBox->text;
        if (levelName.empty()) levelName = "New World";
        std::string levelId = levelName;

        // Simplify level ID
        levelId = Util::stringReplace(levelId, "/", "");
        levelId = Util::stringReplace(levelId, " ", "_");

        int seed = getEpochTimeS();
        if (!seedBox->text.empty()) {
            if (sscanf(seedBox->text.c_str(), "%d", &seed) <= 0) {
                seed = Util::hashCode(seedBox->text);
            }
        }

        LevelSettings settings(seed, isCreative ? GameType::Creative : GameType::Survival);
        minecraft->selectLevel(levelId, levelName, settings);
        minecraft->hostMultiplayer();
        minecraft->setScreen(new ProgressScreen());
    }
}

bool CreateWorldScreen::handleBackEvent(bool isDown) {
    if (!isDown) minecraft->setScreen(NULL);
    return true;
}

void CreateWorldScreen::tick() {
    for (unsigned int i = 0; i < textBoxes.size(); i++) {
        textBoxes[i]->tick();
    }
}
