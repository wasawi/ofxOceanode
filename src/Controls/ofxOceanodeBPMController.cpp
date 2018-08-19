//
//  ofxOceanodeBPMController.cpp
//  example-basic
//
//  Created by Eduard Frigola Bagué on 13/03/2018.
//

#include "ofxOceanodeBPMController.h"
#include "ofxOceanodeContainer.h"
#include <numeric>

//DatGui

ofxOceanodeBPMController::ofxOceanodeBPMController(shared_ptr<ofxOceanodeContainer> _container) : container(_container), ofxOceanodeBaseController("BPM"){
    ofxDatGuiLog::quiet();
    ofxDatGui::setAssetPath("");
    
    mainGuiTheme = new ofxDatGuiThemeCharcoal;
    ofColor randColor =  ofColor::blueSteel;
    mainGuiTheme->color.slider.fill = randColor;
    mainGuiTheme->color.textInput.text = randColor;
    mainGuiTheme->color.icons = randColor;
    int layoutHeight = ofGetWidth()/15;
    mainGuiTheme->font.size = 11;
    mainGuiTheme->layout.height = layoutHeight;
    mainGuiTheme->layout.width = ofGetWidth();
    mainGuiTheme->init();
    
    gui = new ofxDatGui();
    gui->setTheme(mainGuiTheme);
    gui->setPosition(0, 30);
    gui->setWidth(ofGetWidth());
    gui->setAutoDraw(false);
    
    gui->addSlider(bpm.set("BPM", 120, 0, 999));
    gui->addButton("Reset Phase");
    gui->addSlider(phase.set("Phase", 0, 0, 1))->setPrecision(1000);
    gui->addButton("Tap Tempo");
#ifdef OFXOCEANODE_USE_BPM_DETECTION
    useDetection = gui->addToggle("Auto BPM", true);
#endif
    container->setBpm(bpm);
    
    gui->setVisible(false);
    
    //ControlGui Events
    bpmListener = bpm.newListener([&](float &bpm){
        container->setBpm(bpm);
    });
    
    phaseListener = phase.newListener([&](float &ph){
        container->setPhase(ph);
    });
    
    gui->onButtonEvent(this, &ofxOceanodeBPMController::onButtonPress);
    lastButtonPressTime = -1;
    oldBpm = -1;
    
#ifdef OFXOCEANODE_USE_BPM_DETECTION
    
    ofSoundStreamSettings settings;
    
    soundStream.printDeviceList();
    
    auto devices = soundStream.getMatchingDevices("default");
    if(!devices.empty()){
        settings.setInDevice(devices[0]);
    }
    settings.setInListener(this);
    settings.sampleRate = 44100;
    settings.numOutputChannels = 0;
    settings.numInputChannels = 2;
    settings.bufferSize = 512;

    bpmDetection.setup("default",settings.bufferSize,256,settings.sampleRate);

    soundStream.setup(settings);
#endif
}

void ofxOceanodeBPMController::audioIn(ofSoundBuffer &input){
#ifdef OFXOCEANODE_USE_BPM_DETECTION
    bpmDetection.audioIn(input.getBuffer().data(), input.size()/2, input.getNumChannels());
    if(oldBpm != bpmDetection.bpm){
        if(useDetection->getChecked()){
            bpm = bpmDetection.bpm;
        }
    }
    oldBpm = bpmDetection.bpm;
#endif
}

void ofxOceanodeBPMController::onButtonPress(ofxDatGuiButtonEvent e){
    if(e.target->getName() == "Tap Tempo"){
    if(lastButtonPressTime == -1){
        lastButtonPressTime = ofGetElapsedTimef();
    }else{
        float currentTime = ofGetElapsedTimef();
        float lastInterval = currentTime - lastButtonPressTime;
        lastButtonPressTime = currentTime;
        storedIntervals.push_back(lastInterval);
        if(storedIntervals.size() == 1){
            averageInterval = lastInterval;
        }
        else{
            averageInterval = (float)std::accumulate(storedIntervals.begin(), storedIntervals.end(), 0.0) / (float)storedIntervals.size();
            if(lastInterval > averageInterval*2 || lastInterval < averageInterval/2){
                storedIntervals.clear();
            }
            else{
                float newBpm = 60.0/averageInterval;
                if(newBpm != oldBpm)
                    bpm = 60.0/averageInterval;
                oldBpm = newBpm;
            }
        }
    }
    }else if(e.target->getName() == "Reset Phase"){
        container->resetPhase();
    }
}

void ofxOceanodeBPMController::draw(){
    if(isActive)
        gui->draw();
}

void ofxOceanodeBPMController::update(){
    if(isActive)
        gui->update();
}


void ofxOceanodeBPMController::activate(){
    ofxOceanodeBaseController::activate();
    gui->setVisible(true);
}

void ofxOceanodeBPMController::deactivate(){
    ofxOceanodeBaseController::deactivate();
    gui->setVisible(false);
}

void ofxOceanodeBPMController::windowResized(ofResizeEventArgs &a){
    int layoutHeight = ofGetWidth()/15;
    mainGuiTheme->font.size = 11;
    mainGuiTheme->layout.height = layoutHeight;
    mainGuiTheme->layout.width = ofGetWidth();
    mainGuiTheme->init();
    
    gui->setTheme(mainGuiTheme);
    gui->setWidth(ofGetWidth());
}
