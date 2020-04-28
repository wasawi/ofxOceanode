//
//  ofxOceanodeNode.cpp
//  example-basic
//
//  Created by Eduard Frigola on 22/06/2017.
//
//

#include "ofxOceanodeNode.h"
#include "ofxOceanodeContainer.h"
#include "ofxOceanodeNodeModel.h"
#include "ofxOceanodeConnection.h"
#ifndef OFXOCEANODE_HEADLESS
#include "ofxOceanodeNodeGui.h"
#endif

ofxOceanodeNode::ofxOceanodeNode(unique_ptr<ofxOceanodeNodeModel> && _nodeModel) : nodeModel(move(_nodeModel)){
    nodeModelListeners.push(nodeModel->disconnectConnectionsForParameter.newListener([&](string &parameter){
        vector<ofxOceanodeAbstractConnection*> toDeleteConnections;
        //TODO: Change with find_if
        for(auto c : inConnections){
            if(c->getSinkParameter().getName() == parameter) toDeleteConnections.push_back(c);
        }
        for(auto c : outConnections){
            if(c->getSourceParameter().getName() == parameter) toDeleteConnections.push_back(c);
        }
        ofNotifyEvent(deleteConnections, toDeleteConnections);
    }));
    nodeModelListeners.push(nodeModel->deserializeParameterEvent.newListener([this](std::pair<ofJson, string> pair){
        deserializeParameter(pair.first, static_cast<ofxOceanodeAbstractParameter&>(getParameters()->get(pair.second)));
    }));
}

ofxOceanodeNode::~ofxOceanodeNode(){
    
}

void ofxOceanodeNode::setup(){
    nodeModel->setup();
    active = true;
}

//TODO: remove event args
void ofxOceanodeNode::update(ofEventArgs &e){
    nodeModel->update(e);
    nodeGui->update(e);
}

void ofxOceanodeNode::draw(ofEventArgs &e){
    nodeModel->draw(e);
    nodeGui->draw(e);
}

#ifndef OFXOCEANODE_HEADLESS
void ofxOceanodeNode::setGui(std::unique_ptr<ofxOceanodeNodeGui>&& gui){
    nodeGui = std::move(gui);
}

ofxOceanodeNodeGui& ofxOceanodeNode::getNodeGui(){
    return *nodeGui.get();
}
#endif

ofxOceanodeNodeModel& ofxOceanodeNode::getNodeModel(){
    return *nodeModel.get();
}


ofColor ofxOceanodeNode::getColor(){
    return nodeModel->getColor();
}

void ofxOceanodeNode::addOutputConnection(ofxOceanodeAbstractConnection* c){
    outConnections.push_back(c);
    outConnectionsListeners.push(c->destroyConnection.newListener([&, c](){
        outConnections.erase(std::remove(outConnections.begin(), outConnections.end(), c));
    }));
}

void ofxOceanodeNode::addInputConnection(ofxOceanodeAbstractConnection* c){
    inConnections.push_back(c);
    inConnectionsListeners.push(c->destroyConnection.newListener([&, c](){
        inConnections.erase(std::remove(inConnections.begin(), inConnections.end(), c));
    }));
}

void ofxOceanodeNode::deleteSelf(){
    inConnections.insert(inConnections.end(), outConnections.begin(), outConnections.end());
    ofNotifyEvent(deleteModuleAndConnections, inConnections);
}

void ofxOceanodeNode::duplicateSelf(glm::vec2 posToDuplicate){
#ifndef OFXOCEANODE_HEADLESS
    if(posToDuplicate == glm::vec2(-1, -1)){
        posToDuplicate = toGlm(nodeGui->getPosition() + ofPoint(10, 10));
    }
#endif
    saveConfig("tempDuplicateGroup.json");
    ofNotifyEvent(duplicateModule, posToDuplicate);
}

bool ofxOceanodeNode::loadPreset(string presetFolderPath){
    return loadConfig(presetFolderPath + "/" + nodeModel->nodeName() + "_" + ofToString(nodeModel->getNumIdentifier()) + ".json");
}

void ofxOceanodeNode::savePreset(string presetFolderPath){
    saveConfig(presetFolderPath + "/" + nodeModel->nodeName() + "_" + ofToString(nodeModel->getNumIdentifier()) + ".json");
}

bool ofxOceanodeNode::loadPersistentPreset(string presetFolderPath){
    return loadConfig(presetFolderPath + "/" + nodeModel->nodeName() + "_" + ofToString(nodeModel->getNumIdentifier()) + ".json", true);
}

void ofxOceanodeNode::savePersistentPreset(string presetFolderPath){
    saveConfig(presetFolderPath + "/" + nodeModel->nodeName() + "_" + ofToString(nodeModel->getNumIdentifier()) + ".json", true);
}

void ofxOceanodeNode::presetWillBeLoaded(){
    nodeModel->presetWillBeLoaded();
}

void ofxOceanodeNode::presetHasLoaded(){
    nodeModel->presetHasLoaded();
}

void ofxOceanodeNode::loadPresetBeforeConnections(string presetFolderPath){
    string filename = presetFolderPath + "/" + nodeModel->nodeName() + "_" + ofToString(nodeModel->getNumIdentifier()) + ".json";
    string escapedFilename = filename;
    ofStringReplace(escapedFilename, " ", "_");
    ofJson json = ofLoadJson(escapedFilename);
    if(json.empty()) json = ofLoadJson(filename);
    
    if(json.empty()) return;
    
    nodeModel->loadBeforeConnections(json);
}

bool ofxOceanodeNode::loadConfig(string filename, bool persistentPreset){
    string escapedFilename = filename;
    ofStringReplace(escapedFilename, " ", "_");
    ofJson json = ofLoadJson(escapedFilename);
    if(json.empty()) json = ofLoadJson(filename);
    
    if(json.empty()) return false;
    
    if(persistentPreset)
        nodeModel->loadCustomPersistent(json);
    
    //Hack Put all faders to 0;
//    if(getParameters()->contains("Fader")){
//        if(getParameters()->get("Fader").type() == typeid(ofParameter<float>()).name()){
//            getParameters()->getFloat("Fader") = 0;
//        }else{
//            getParameters()->get<vector<float>>("Fader") = {0};
//        }
//    }
    
    nodeModel->presetRecallBeforeSettingParameters(json);
    loadParametersFromJson(json, persistentPreset);
    nodeModel->presetRecallAfterSettingParameters(json);
    return true;
}

void ofxOceanodeNode::saveConfig(string filename, bool persistentPreset){
    ofStringReplace(filename, " ", "_");
    ofJson json = saveParametersToJson(persistentPreset);
    nodeModel->presetSave(json);
    ofSavePrettyJson(filename, json);
}

ofJson ofxOceanodeNode::saveParametersToJson(bool persistentPreset){
    ofJson json;
    for(int i = 0; i < getParameters()->size(); i++){
        ofxOceanodeAbstractParameter& p = static_cast<ofxOceanodeAbstractParameter&>(getParameters()->get(i));
        if((!persistentPreset && !(p.getFlags() & ofxOceanodeParameterFlags_DisableSavePreset)) || (persistentPreset && !(p.getFlags() & ofxOceanodeParameterFlags_DisableSaveProject))){
            if(p.valueType() == typeid(float).name()){
                ofSerialize(json, p.cast<float>().getParameter());
            }else if(p.valueType() == typeid(int).name()){
                ofSerialize(json, p.cast<int>().getParameter());
            }
            else if(p.valueType() == typeid(bool).name()){
                ofSerialize(json, p.cast<bool>().getParameter());
            }
            else if(p.valueType() == typeid(ofColor).name()){
                ofSerialize(json, p.cast<ofColor>().getParameter());
            }
            else if(p.valueType() == typeid(string).name()){
                ofSerialize(json, p.cast<string>().getParameter());
            }
            else if(p.valueType() == typeid(vector<float>).name()){
                auto vecF = p.cast<vector<float>>().getParameter().get();
                if(vecF.size() == 1){
                    json[p.getEscapedName()] = vecF[0];
                }
            }
            else if(p.type() == typeid(vector<int>).name()){
                auto vecI = p.cast<vector<int>>().getParameter().get();
                if(vecI.size() == 1){
                    json[p.getEscapedName()] = vecI[0];
                }
            }
        }
    }
    return json;
}
bool ofxOceanodeNode::loadParametersFromJson(ofJson json, bool persistentPreset){
    for (ofJson::iterator it = json.begin(); it != json.end(); ++it) {
        if(getParameters()->contains(it.key())){
            ofxOceanodeAbstractParameter& p = static_cast<ofxOceanodeAbstractParameter&>(getParameters()->get(it.key()));
            deserializeParameter(json, p, persistentPreset);
        }
    }
    return true;
}

void ofxOceanodeNode::deserializeParameter(ofJson &json, ofxOceanodeAbstractParameter &p, bool persistentPreset){
    if((((!persistentPreset && !(p.getFlags() & ofxOceanodeParameterFlags_DisableSavePreset)) || (persistentPreset && !(p.getFlags() & ofxOceanodeParameterFlags_DisableSaveProject)))) && json.count(p.getEscapedName()) && !checkHasInConnection(p)){
        if(p.valueType() == typeid(float).name()){
            ofDeserialize(json, p.cast<float>().getParameter());
        }else if(p.valueType() == typeid(int).name()){
            ofDeserialize(json, p.cast<int>().getParameter());
        }
        else if(p.valueType() == typeid(bool).name()){
            ofDeserialize(json, p.cast<bool>().getParameter());
        }
        else if(p.valueType() == typeid(ofColor).name()){
            ofDeserialize(json, p.cast<ofColor>().getParameter());
        }
        else if(p.valueType() == typeid(string).name()){
            ofDeserialize(json, p.cast<string>().getParameter());
        }
        else if(p.valueType() == typeid(vector<float>).name()){
            float value = 0;
            if(json[p.getEscapedName()].is_string()){
				p.cast<vector<float>>().getParameter() = vector<float>(1, ofToFloat(json[p.getEscapedName()]));
            }else{
                p.cast<vector<float>>().getParameter() = vector<float>(1, float(json[p.getEscapedName()]));
            }
        }
        else if(p.valueType() == typeid(vector<int>).name()){
            if(json[p.getEscapedName()].is_string()){
                p.cast<vector<int>>().getParameter() = vector<int>(1, ofToInt(json[p.getEscapedName()]));
            }else{
                p.cast<vector<int>>().getParameter() = vector<int>(1, int(json[p.getEscapedName()]));
            }
        }
    }
}

void ofxOceanodeNode::setBpm(float bpm){
    nodeModel->setBpm(bpm);
}

void ofxOceanodeNode::resetPhase(){
    nodeModel->resetPhase();
}

bool ofxOceanodeNode::checkHasInConnection(ofAbstractParameter &p){
    for(auto &c : inConnections){
        if(&c->getSinkParameter() == &p){
            return true;
        }
    }
    return false;
}

ofxOceanodeAbstractConnection* ofxOceanodeNode::getInputConnectionForParameter(ofAbstractParameter& param){
    auto findPos = std::find_if(inConnections.begin(), inConnections.end(), [&param](ofxOceanodeAbstractConnection *c){
        return
        &c->getSinkParameter() == &param;
    });
    if(findPos != inConnections.end()){
        return *findPos;
    }
    return nullptr;
}

ofxOceanodeAbstractConnection* ofxOceanodeNode::getOutputConnectionForParameter(ofAbstractParameter& param){
    auto findPos = std::find_if(outConnections.begin(), outConnections.end(), [&param](ofxOceanodeAbstractConnection *c){
        return
        &c->getSinkParameter() == &param;
    });
    if(findPos != outConnections.end()){
        return *findPos;
    }
    return nullptr;
}

shared_ptr<ofParameterGroup> ofxOceanodeNode::getParameters(){
    return nodeModel->getParameterGroup();
}

void ofxOceanodeNode::setActive(bool act){
    if(act == active) return;
    active = act;
#ifndef OFXOCEANODE_HEADLESS
    if(active){
        nodeGui->enable();
    }else{
        nodeGui->disable();
    }
#endif
}
