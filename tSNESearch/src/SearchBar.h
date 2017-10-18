#pragma once

#include "ofMain.h"

class SearchBar {
protected:
    ofTrueTypeFont font;
    string value;
    int position;
    ofRectangle box;
    ofVec2f cursorPosition;
    ofVec2f cursorOffset;
    uint64_t lastTime;
    void updateCursor() {
        string substr = value.substr(0, position) + ".";
        box = font.getStringBoundingBox(substr, 0, 0);
        cursorPosition.x = box.width;
    }
public:
    ofEvent<string> valueChange;
    SearchBar()
    :position(0)
    ,cursorOffset(-4, 0) {
    }
    void setup(int fontSize) {
        font.load(OF_TTF_SANS, fontSize);
        ofAddListener(ofEvents().keyPressed, this, &SearchBar::keyPressed);
        updateCursor();
    }
    const string& getValue() const {
        return value;
    }
    int size() const {
        return value.size();
    }
    void draw(float x, float y) {
        if(size()) {
            ofPushMatrix();
            ofTranslate(x - box.width / 2, y);
            font.drawString(value, 0, 0);
            uint64_t timeDiff = ofGetElapsedTimeMillis() - lastTime;
            if(timeDiff % 1500 < 1000) {
                float localX = .5 + int(cursorPosition.x + cursorOffset.x);
                ofDrawLine(localX, cursorOffset.y - font.getSize(), localX, cursorOffset.y - font.getDescenderHeight());
            }
            ofPopMatrix();
        }
    }
    void keyPressed(ofKeyEventArgs& key) {
        if(key.key == ' ') {
            return;
        }
        string valueBefore = value;
        if(key.key == OF_KEY_LEFT) {
            position--;
        } else if(key.key == OF_KEY_RIGHT) {
            position++;
        } else if(key.key == OF_KEY_BACKSPACE) {
            if(position > 0) {
                value.erase(value.begin() + position - 1);
                position--;
            }
        } else if(key.key == OF_KEY_DEL) {
            if(position < value.length()) {
                value.erase(value.begin() + position);
            }
        } else if(isprint(key.key)) {
            stringstream character;
            character << char(key.key);
            value.insert(position, character.str());
            position++;
        }
        position = MAX(0, position);
        position = MIN(value.length(), position);
        lastTime = ofGetElapsedTimeMillis();
        updateCursor();
        if(valueBefore != value) {
            ofNotifyEvent(valueChange, value, this);
        }
    }
};