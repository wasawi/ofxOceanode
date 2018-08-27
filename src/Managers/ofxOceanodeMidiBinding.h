//
//  ofxOceanodeMidiBinding.h
//  example-midi
//
//  Created by Eduard Frigola Bagué on 20/08/2018.
//

#ifndef ofxOceanodeMidiBinding_h
#define ofxOceanodeMidiBinding_h

#ifdef OFXOCEANODE_USE_MIDI

#include "ofParameter.h"
#include "ofJson.h"
#include "ofxMidi.h"

using namespace std;

class ofxOceanodeAbstractMidiBinding : public ofxMidiListener{
public:
    ofxOceanodeAbstractMidiBinding(){
        isListening = true;
        messageType.set("Message Type", "Not Assigned");
        channel.set("Channel", -1, 1, 16);
        control.set("Control", -1, 0, 127);
        value.set("Value", -1, 0, 127);
    };
    ~ofxOceanodeAbstractMidiBinding(){};
    
    virtual void newMidiMessage(ofxMidiMessage& message) {
        portName = message.portName;
        channel = message.channel;
        status = message.status;
        messageType = ofxMidiMessage::getStatusString(status);
        if(message.status == MIDI_CONTROL_CHANGE){
            control = message.control;
        }
        if(message.status == MIDI_NOTE_ON || message.status == MIDI_NOTE_OFF){
            control = message.pitch;
        }
        isListening = false;
        unregisterUnusedMidiIns.notify(this, portName);
    }
    
    virtual void savePreset(ofJson &json){
        json["Channel"] = channel.get();
        json["Control"] = control.get();
        json["Type"] = messageType.get();
    }
    
    virtual void loadPreset(ofJson &json){
        isListening = false;
        channel.set(json["Channel"]);
        control.set(json["Control"]);
        messageType.set(json["Type"]);
        if(messageType.get() == ofxMidiMessage::getStatusString(MIDI_CONTROL_CHANGE)) status = MIDI_CONTROL_CHANGE;
        else if(messageType.get() == ofxMidiMessage::getStatusString(MIDI_NOTE_ON)) status = MIDI_NOTE_ON;
        else ofLog() << "Type Error";
    }
    
    string type() const{
        return typeid(*this).name();
    }

    string getName(){return name;};
    
    ofParameter<string> &getMessageType(){return messageType;};
    ofParameter<int> &getChannel(){return channel;};
    ofParameter<int> &getControl(){return control;};
    ofParameter<int> &getValue(){return value;};
    
    virtual ofxMidiMessage& sendMidiMessage(){};
    virtual void bindParameter(){};
    
    ofEvent<string> unregisterUnusedMidiIns;
    ofEvent<ofxMidiMessage> midiMessageSender;
protected:
    bool isListening;
    
    string portName;
    ofParameter<string> messageType;
    MidiStatus status;
    ofParameter<int> channel;
    ofParameter<int> control;
    ofParameter<int> value;
    
    string name;
    ofxMidiMessage firstMidiMessage;
    
    bool modifiyingParameter;
    ofEventListener listener;
};


template<typename T, typename Enable = void>
class ofxOceanodeMidiBinding : public ofxOceanodeAbstractMidiBinding{
public:
    ofxOceanodeMidiBinding(ofParameter<T>& _parameter) : parameter(_parameter), ofxOceanodeAbstractMidiBinding(){
        name = parameter.getGroupHierarchyNames()[0] + "-|-" + parameter.getEscapedName();
    }
    
    ~ofxOceanodeMidiBinding(){};
    
    void newMidiMessage(ofxMidiMessage& message){};
    void bindParameter(){};
    
private:
    ofParameter<T>& parameter;
};

template<typename T>
class ofxOceanodeMidiBinding<T, typename std::enable_if<std::is_same<T, int>::value || std::is_same<T, float>::value>::type>: public ofxOceanodeAbstractMidiBinding{
public:
    ofxOceanodeMidiBinding(ofParameter<T>& _parameter) : parameter(_parameter), ofxOceanodeAbstractMidiBinding(){
        name = parameter.getGroupHierarchyNames()[0] + "-|-" + parameter.getEscapedName();
        min.set("Min", parameter.getMin(), parameter.getMin(), parameter.getMax());
        max.set("Max", parameter.getMax(), parameter.getMin(), parameter.getMax());
    }

    ~ofxOceanodeMidiBinding(){};
    
    void savePreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::savePreset(json);
        json["Min"] = min.get();
        json["Max"] = max.get();
    }
    
    void loadPreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::loadPreset(json);
        min.set(json["Min"]);
        max.set(json["Max"]);
    }

    void newMidiMessage(ofxMidiMessage& message){
        if(message.status == MIDI_NOTE_OFF){
            message.status = MIDI_NOTE_ON;
            message.velocity = 0;
        }
        if(isListening) ofxOceanodeAbstractMidiBinding::newMidiMessage(message);
        if(message.channel == channel && message.status == status){
            bool validMessage = false;
            switch(status){
                case MIDI_CONTROL_CHANGE:
                {
                    if(message.control == control){
                        value = message.value;
                        validMessage = true;
                    }
                    break;
                }
                case MIDI_NOTE_ON:
                {
                    if(message.pitch == control){
                        value = message.velocity;
                        validMessage = true;
                        
                    }
                    break;
                }
                default:
                {
                    ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type " << typeid(T).name();
                }
            }
            if(validMessage){
                modifiyingParameter = true;
                parameter.set(ofMap(value, 1, 127, min, max, true));
                modifiyingParameter = false;
            }
        }
        
    };
    void bindParameter(){
        listener = parameter.newListener([this](T &f){
            if(!modifiyingParameter){
                value = ofMap(f, min, max, 1, 127, true);
                ofxMidiMessage message;
                message.status = status;
                message.channel = channel;
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                        message.control = control;
                        message.value = value;
                        break;
                    case MIDI_NOTE_ON:
                        message.pitch = control;
                        message.velocity = value;
                        break;
                    default:
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type " << typeid(T).name();
                }
                midiMessageSender.notify(this, message);
            }
        });
    };
    
    ofParameter<T> &getMinParameter(){return min;};
    ofParameter<T> &getMaxParameter(){return max;};

private:
    ofParameter<T>& parameter;
    ofParameter<T> min;
    ofParameter<T> max;
    
};

template<typename>
struct is_std_vector2 : std::false_type {};

template<typename T, typename A>
struct is_std_vector2<std::vector<T,A>> : std::true_type {};

template<typename T>
class ofxOceanodeMidiBinding<vector<T>> : public ofxOceanodeAbstractMidiBinding{
public:
    ofxOceanodeMidiBinding(ofParameter<vector<T>>& _parameter) : parameter(_parameter), ofxOceanodeAbstractMidiBinding(){
        name = parameter.getGroupHierarchyNames()[0] + "-|-" + parameter.getEscapedName();
        min.set("Min", parameter.getMin()[0], parameter.getMin()[0], parameter.getMax()[0]);
        max.set("Max", parameter.getMax()[0], parameter.getMin()[0], parameter.getMax()[0]);
    }
    
    ~ofxOceanodeMidiBinding(){};
    
    void savePreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::savePreset(json);
        json["Min"] = min.get();
        json["Max"] = max.get();
    }
    
    void loadPreset(ofJson &json){
        ofxOceanodeAbstractMidiBinding::loadPreset(json);
        min.set(json["Min"]);
        max.set(json["Max"]);
    }
    
    void newMidiMessage(ofxMidiMessage& message){
        if(message.status == MIDI_NOTE_OFF){
            message.status = MIDI_NOTE_ON;
            message.velocity = 0;
        }
        if(isListening) ofxOceanodeAbstractMidiBinding::newMidiMessage(message);
        if(message.channel == channel && message.status == status){
            bool validMessage = false;
            switch(status){
                case MIDI_CONTROL_CHANGE:
                {
                    if(message.control == control){
                        value = message.value;
                        validMessage = true;
                    }
                    break;
                }
                case MIDI_NOTE_ON:
                {
                    if(message.pitch == control){
                        value = message.velocity;
                        validMessage = true;
                    }
                    break;
                }
                default:
                {
                    ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type " << typeid(T).name();
                }
            }
            if(validMessage){
                modifiyingParameter = true;
                    parameter.set(vector<T>(1, ofMap(value, 1, 127, min, max, true)));
                modifiyingParameter = false;
            }
        }
    };
    
    void bindParameter(){
        listener = parameter.newListener([this](vector<T> &vf){
            if(!modifiyingParameter && vf.size() == 1){
                value = ofMap(vf[0], min, max, 1, 127, true);
                ofxMidiMessage message;
                message.status = status;
                message.channel = channel;
                switch(status){
                    case MIDI_CONTROL_CHANGE:
                        message.control = control;
                        message.value = value;
                        break;
                    case MIDI_NOTE_ON:
                        message.pitch = control;
                        message.velocity = value;
                        break;
                    default:
                        ofLog() << "Midi Type " << ofxMidiMessage::getStatusString(message.status) << " not supported for parameter of type " << typeid(T).name();
                }
                midiMessageSender.notify(this, message);
            }
        });
    };
    
    ofParameter<T> &getMinParameter(){return min;};
    ofParameter<T> &getMaxParameter(){return max;};
    
private:
    ofParameter<vector<T>>& parameter;
    ofParameter<T> min;
    ofParameter<T> max;
};

#endif

#endif /* ofxOceanodeMidiBinding_h */
