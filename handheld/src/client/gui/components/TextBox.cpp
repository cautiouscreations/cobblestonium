#include "TextBox.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../Font.h"
#include "../../../platform/input/Keyboard.h"

TextBox::TextBox( int id, const std::string& msg )
 : id(id), w(0), h(0), x(0), y(y), text(msg), focused(false), cursor(0), tickCount(0) {
}

TextBox::TextBox( int id, int x, int y, const std::string& msg ) 
 : id(id), w(0), h(0), x(x), y(y), text(msg), focused(false), cursor(0), tickCount(0) {
}

TextBox::TextBox( int id, int x, int y, int w, int h, const std::string& msg )
 : id(id), w(w), h(h), x(x), y(y), text(msg), focused(false), cursor(0), tickCount(0) {
}

void TextBox::setFocus(Minecraft* minecraft) {
	if(!focused) {
		minecraft->platform()->showKeyboard();
		focused = true;
	}
}

bool TextBox::loseFocus(Minecraft* minecraft) {
	if(focused) {
		minecraft->platform()->hideKeyboard();
		focused = false;
		return true;
	}
	return false;
}

void TextBox::setText(const std::string& text) {
    this->text = text;
    cursor = text.length();
}

void TextBox::tick() {
    tickCount++;
}

void TextBox::render( Minecraft* minecraft, int xm, int ym ) {
    fill(x - 1, y - 1, x + w + 1, y + h + 1, 0xffa0a0a0);
    fill(x, y, x + w, y + h, 0xff000000);

    int color = 0xffe0e0e0;
    std::string renderedText = text;
    if (focused && (tickCount / 6) % 2 == 0) {
        renderedText += "_";
    }

    drawString(minecraft->font, renderedText, x + 4, y + (h - 8) / 2, color);
}

void TextBox::keyPressed(Minecraft* minecraft, int eventKey) {
    if (!focused) return;

    if (eventKey == Keyboard::KEY_BACKSPACE) {
        if (text.length() > 0) {
            text.pop_back();
        }
    } else if (eventKey == Keyboard::KEY_RETURN) {
        loseFocus(minecraft);
    }
}

void TextBox::keyboardNewChar(Minecraft* minecraft, char inputChar) {
    if (!focused) return;
    if (inputChar >= 32 && inputChar <= 126) {
        text += inputChar;
    }
}
