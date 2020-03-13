//
//  ofxOceanodeControls.h
//  example-basic
//
//  Created by Eduard Frigola Bagué on 12/03/2018.
//

#ifndef ofxOceanodeControls_h
#define ofxOceanodeControls_h

#ifndef OFXOCEANODE_HEADLESS

#include "ofMain.h"
#include "ofxOceanodeBaseController.h"

class ofxOceanodeContainer;

class ofxOceanodeControls{
public:
    ofxOceanodeControls(shared_ptr<ofxOceanodeContainer> _container);
    ~ofxOceanodeControls(){};
    
    void draw(ofEventArgs &e);
    void update(ofEventArgs &e);
    void exit(ofEventArgs &e);
    
    void keyPressed(ofKeyEventArgs &e){};
    void mouseMoved(ofMouseEventArgs &a);
    void mousePressed(ofMouseEventArgs &a);
    void mouseReleased(ofMouseEventArgs &a);
    void mouseDragged(ofMouseEventArgs &a);
    void windowResized(ofResizeEventArgs &a);
    
    template<class T>
    shared_ptr<T> get(){
        for(auto c : controllers){
            if(dynamic_pointer_cast<T>(c) != nullptr){
                return dynamic_pointer_cast<T>(c);
            }
        }
        return nullptr;
    }
    
    void resizeButtons();
    
    ofRectangle getWindowRect(){
        if(windowRect != ofRectangle(0,0,0,0))
            return windowRect;
        
        return ofRectangle(controlsWindow->getWindowPosition(), controlsWindow->getWindowSize());
        
    };
    void setWindowRect(ofRectangle rect){
        controlsWindow->setWindowPosition(rect.x, rect.y);
        controlsWindow->setWindowShape(rect.width, rect.height);
    }
    
private:
    std::shared_ptr<ofAppBaseWindow> controlsWindow;
    std::shared_ptr<ofxOceanodeContainer> container;
    
    ofEventListeners listeners;
    
    vector<shared_ptr<ofxOceanodeBaseController>> controllers;
    vector<ofRectangle> controllersButtons;
    
    ofRectangle windowRect;
};

#endif

#endif /* ofxOceanodeControls_h */
