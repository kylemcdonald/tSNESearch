#include "ofMain.h"
#include "SearchBar.h"

#define MAX_SOUNDS 32

bool donePlaying(const ofSoundPlayer& sound) {
    return !(sound.isPlaying()) || (sound.getPositionMS() > 1000);
}

class ofApp : public ofBaseApp {
public:
    string splitString = "/";
    int pathOffset = 0;
    
    string tsnePath;
    int tsneOffset = 0;
    string tsneSource;
    SearchBar search;
    string recentFilename;
    vector<string> filesBasename, filesFullname;
    vector<string> searchable;
    vector<bool> valid;
    ofVboMesh mesh, meshSearch;
    int nearestIndex, nearestIndexPrev;
    vector<ofSoundPlayer> sounds;
    ofTrueTypeFont font;
    vector<int> indices;
    
    struct AssignedSound {
        shared_ptr<ofSoundBuffer> buffer;
        ofVec2f position;
    };
    
    void updateSounds() {
        ofRemove(sounds, donePlaying);
    }
    void keyPressed(int key) override {
        if(key == OF_KEY_RIGHT) {
            tsneOffset++;
            loadNextTsne();
        }
        if(key == OF_KEY_LEFT) {
            tsneOffset--;
            tsneOffset = MAX(tsneOffset, 0);
            loadNextTsne();
        }
        if(key == OF_KEY_RETURN) {
            ofSystem("open \"" + recentFilename + "\"");
        }
    }
    void playSound(string filename) {
        if(sounds.size() < MAX_SOUNDS) {
            recentFilename = filename;
            sounds.emplace_back();
            sounds.back().load(filename, true);
            sounds.back().play();
        }
    }
    void valueChange(string& value) {
        string x = ofToLower(value);
        meshSearch.clearColors();
        valid.clear();
        int visible = 0;
        for(int i = 0; i < searchable.size(); i++) {
            const string& terms = searchable[i];
            ofFloatColor base = mesh.getColors()[i];
            if(terms.find(x) != std::string::npos) {
                valid.push_back(true);
                visible++;
            } else {
                base.a = 0;
                valid.push_back(false);
            }
            meshSearch.addColor(base);
        }
    }

    void setup() override {
        ofBackground(0);
        ofSetVerticalSync(false);
        glPointSize(4);
        ofSetLineWidth(3);
        mesh.setMode(OF_PRIMITIVE_POINTS);
        font.load(OF_TTF_SANS, 32);
        search.setup(48);
        ofAddListener(search.valueChange, this, &ofApp::valueChange);
        
        string source;
        ofBuffer settingsFile = ofBufferFromFile("settings.yml");
        for(auto& line : settingsFile.getLines()) {
            if(line.size()) {
                vector<string> parts = ofSplitString(line, ":");
                string key = parts[0], value = ofTrim(parts[1]);
                if(key == "source") {
                    source = value;
                }
                if(key == "pathOffset") {
                    pathOffset = ofToInt(value);
                }
                if(key == "splitString") {
                    splitString = value;
                }
            }
        }
        
        tsnePath = ofFilePath::join(source, "tsne");
        
        string filenamesPath = ofFilePath::join(source, "filenames.txt");
        ofBuffer filenames = ofBufferFromFile(filenamesPath);
        for(auto& line : filenames.getLines()) {
            if(line.size()) {
                int lastSlash = line.find_last_of("/");
                string result = line.substr(lastSlash + 1);
                filesBasename.push_back(result);
                filesFullname.push_back(line);
            }
        }
        
        string searchablePath = ofFilePath::join(source, "searchable.txt");
        if(ofFile(searchablePath).exists()) {
            ofBuffer searchables = ofBufferFromFile(searchablePath);
            for(auto& line : searchables.getLines()) {
                searchable.push_back(ofToLower(line));
            }
        } else {
            for(auto& line : filesFullname) {
                vector<string> parts = ofSplitString(line, splitString);
                string suffix = ofJoinString(vector<string>(parts.begin() + pathOffset, parts.end()), splitString);
                searchable.push_back(ofToLower(suffix));
            }
        }
        
        loadNextTsne();
    }
    void loadNextTsne() {
        ofDirectory files;
        files.allowExt("tsv");
        files.allowExt("tsne");
        files.listDir(tsnePath);
        
        int i = (tsneOffset * 2) % files.size();
        ofFile path2d = files[i];
        ofFile path3d = files[i+1];
        
        ofLog() << i << " of " << files.size();
        ofLog() << path2d.getAbsolutePath();
        ofLog() << path3d.getAbsolutePath();
        
        tsneSource = path2d.getBaseName();
        ofStringReplace(tsneSource, ".2d", "");
        
        mesh.clear();
        
        ofBuffer tsne2d = ofBufferFromFile(path2d.getAbsolutePath());
        for(auto& line : tsne2d.getLines()) {
            if(line.size()) {
                ofVec2f x;
                stringstream(line) >> x;
//                x.x += ofRandomuf() * .01;
//                x.y += ofRandomuf() * .01;
                mesh.addVertex(x);
            }
        }
        
        ofBuffer tsne3d = ofBufferFromFile(path3d.getAbsolutePath());
        for(auto& line : tsne3d.getLines()) {
            if(line.size()) {
                ofVec3f x;
                stringstream(line) >> x;
                mesh.addColor(ofFloatColor(x.x, x.y, x.z));
            }
        }
        
        meshSearch = mesh;
    }
    void update() override {
        updateSounds();
    }
    void draw() override {
        float scale = ofGetHeight();
        float offset = (ofGetWidth() - scale) / 2;
        
        ofPushMatrix();
        ofTranslate(offset, 0);
        ofScale(scale, scale);
        if(search.size()) {
            meshSearch.draw();
        } else {
            mesh.draw();
        }
        ofPopMatrix();
        
        search.draw(ofGetWidth() / 2, ofGetHeight() / 2);
        
        float nearestDistance = -1;
        nearestIndex = 0;
        ofVec2f mouse(mouseX - offset, mouseY);
        mouse /= scale;
        for(int i = 0; i < mesh.getNumVertices(); i++) {
            if(search.size() == 0 || valid.size() == 0 || valid[i]) {
                ofVec3f& x = mesh.getVertices()[i];
                float distance = mouse.squareDistance(x);
                if(nearestDistance < 0 || distance < nearestDistance) {
                    nearestDistance = distance;
                    nearestIndex = i;
                }
            }
        }
        
//        font.drawString(tsneSource, 10, ofGetHeight() - 40);
        
        float minDistance = 100 / scale;
        minDistance = minDistance * minDistance;
        if(nearestDistance < minDistance) {
            ofVec2f position = mesh.getVertex(nearestIndex) * scale;
            ofPushMatrix();
            ofTranslate(offset, 0);
            ofNoFill();
            ofDrawCircle(position, 10);
            ofPopMatrix();
            
            vector<string> parts = ofSplitString(searchable[nearestIndex], splitString);
            if(parts.size()) {
//                reverse(parts.begin(), parts.end());
                string joined = ofJoinString(parts, "\n");
                font.drawString(joined, 10, 40);
//                font.drawString(filesFullname[nearestIndex], 10, 40);
            }
            if(nearestIndex != nearestIndexPrev) {
                playSound(filesFullname[nearestIndex]);
            }
            nearestIndexPrev = nearestIndex;
        }
    }
};

int main() {
    ofSetupOpenGL(1024, 1024, OF_FULLSCREEN);
    ofRunApp(new ofApp());
}
