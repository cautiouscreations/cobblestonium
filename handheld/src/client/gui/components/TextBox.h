#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__

//package net.minecraft.client.gui;

#include <string>
#include "../GuiComponent.h"
#include "../../Options.h"

class Font;
class Minecraft;

class TextBox: public GuiComponent
{
public:
	TextBox(int id, const std::string& msg);
    TextBox(int id, int x, int y, const std::string& msg);
    TextBox(int id, int x, int y, int w, int h, const std::string& msg);

    virtual void setFocus(Minecraft* minecraft);
	virtual bool loseFocus(Minecraft* minecraft);

    virtual void render(Minecraft* minecraft, int xm, int ym);
	virtual void keyPressed(Minecraft* minecraft, int eventKey);
    virtual void keyboardNewChar(Minecraft* minecraft, char inputChar);
    virtual void tick();

    void setText(const std::string& text);
	
public:
	int w, h;
	int x, y;

	std::string text;
	int id;
	bool focused;
    int cursor;
    int tickCount;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__*/
