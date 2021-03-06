//
//  ofxOceanodeNodeMacro.h
//  example-basic
//
//  Created by Eduard Frigola Bagué on 20/06/2019.
//

#ifndef ofxOceanodeNodeMacro_h
#define ofxOceanodeNodeMacro_h

#include "ofxOceanode.h"
#include "ofxOceanodeNodeModelExternalWindow.h"

template<typename T, typename Enable = void>
class router : public ofxOceanodeNodeModel{
public:
    router(string name) : ofxOceanodeNodeModel(name){};// + " " + typeid(T).name()){};
    void setup(){
        setupValueParameter();
        addParameter(value);
		addParameter(min.set("Min", "0"), ofxOceanodeParameterFlags_DisableInConnection);
		addParameter(max.set("Max", "1"), ofxOceanodeParameterFlags_DisableInConnection);
    }
    
    void setupValueParameter(){
        value.set({0});
        value.setMin({0});
        value.setMax({1});
    }
    
protected:
    ofParameter<T> value;
    ofParameter<string> min;
    ofParameter<string> max;
};

template<>
class router<ofTexture*> : public ofxOceanodeNodeModel{
public:
    router(string name) : ofxOceanodeNodeModel(name + " Tex"){};// + " " + typeid(T).name()){};
    void setup(){
		setupValueParameter();
       	addParameter(value);
    }
    
    void setupValueParameter(){
        value.set(nullptr);
    }
    
protected:
    ofParameter<ofTexture*> value;
};

template<typename T>
class inlet : public router<T>{
public:
    inlet() : router<T>("Inlet"){};
    void setup(){
        this->value.setName("Input");
        router<T>::setup();
        //this->valueInfo->acceptInConnection = false;
    }
};

template<typename T>
class outlet : public router<T>{
public:
    outlet() : router<T>("Outlet"){};
    void setup(){
        this->value.setName("Output");
        router<T>::setup();
        //this->valueInfo->acceptOutConnection = false;
    }
};

class ofxOscMessage;

class ofxOceanodeNodeMacro : public ofxOceanodeNodeModel{
public:
    ofxOceanodeNodeMacro();
    ~ofxOceanodeNodeMacro(){};
    
    void setup();
    void update(ofEventArgs &a);
    void draw(ofEventArgs &a);
    
    void setContainer(ofxOceanodeContainer* container);
    
    void presetSave(ofJson &json);
    void loadBeforeConnections(ofJson &json);
    
    void setBpm(float bpm){container->setBpm(bpm);};
    void resetPhase(){container->resetPhase();};
    
#ifdef OFXOCEANODE_USE_OSC
    bool receiveOscMessage(ofxOscMessage &m) override{
        container->receiveOscMessage(m);
        return true;};
#endif
    
private:
    void newNodeCreated(ofxOceanodeNode* &node);
    
#ifndef OFXOCEANODE_HEADLESS
    ofxOceanodeCanvas canvas;
#endif
    shared_ptr<ofxOceanodeContainer> container;
    shared_ptr<ofxOceanodeNodeRegistry> registry;
    shared_ptr<ofxOceanodeTypesRegistry> typesRegistry;
    
    ofEventListener newNodeListener;
    std::unordered_map<string, ofAbstractParameter*> paramsStore;
    std::unordered_map<string, ofEventListeners> inoutListeners;
    ofEventListeners deleteListeners;
    
    ofParameter<bool> showWindow;
    string presetPath;
    ofParameter<int> bank;
    int previousBank;
    ofAbstractParameter* bankDropdown;
    vector<string> bankNames;
    ofParameter<int> preset;
    ofAbstractParameter* presetDropdown;
    int currentPreset;
    ofParameter<string> savePresetField;
    vector<string> presetsInBank;
    ofParameter<string> presetName;
    ofParameter<bool> savePreset;
    
    std::unordered_map<string, std::time_t> presetLastChanged;
    std::time_t bankLastChanged;
    std::time_t presetsInBankLastChanged;
    
    ofEventListeners presetActionsListeners;
    
    string canvasParentID;
};

#endif /* ofxOceanodeNodeMacro_h */
