//
//  ofxOceanodeContainer.h
//  example-basic
//
//  Created by Eduard Frigola on 19/06/2017.
//
//

#ifndef ofxOceanodeContainer_h
#define ofxOceanodeContainer_h

#include "ofxOceanodeConnection.h"
#include "ofxOceanodeNode.h"
#ifndef OFXOCEANODE_HEADLESS
#include "ofxOceanodeNodeGui.h"
#endif

class ofxOceanodeNodeModel;
class ofxOceanodeNodeRegistry;
class ofxOceanodeTypesRegistry;

#ifdef OFXOCEANODE_USE_OSC
#include "ofxOsc.h"
#endif

#ifdef OFXOCEANODE_USE_MIDI
class ofxOceanodeAbstractMidiBinding;
class ofxMidiIn;
class ofxMidiOut;
class ofxMidiListener;
#endif


class ofxOceanodeContainer {
public:
    using nodeContainerWithId = std::unordered_map<int, unique_ptr<ofxOceanodeNode>>;
    
    ofxOceanodeContainer(std::shared_ptr<ofxOceanodeNodeRegistry> _registry = nullptr, std::shared_ptr<ofxOceanodeTypesRegistry> _typesRegistry = nullptr);
    ~ofxOceanodeContainer();
    
    void clearContainer();
    
    void update();
    void draw();
    
    
    ofxOceanodeNode* createNodeFromName(string name, int identifier = -1, bool isPersistent = false);
    ofxOceanodeNode& createNode(unique_ptr<ofxOceanodeNodeModel> && nodeModel, int identifier = -1, bool isPersistent = false);
    
    template<typename ModelType>
    ofxOceanodeNode& createPersistentNode(){
        static_assert(std::is_base_of<ofxOceanodeNodeModel, ModelType>::value,
                      "Must pass a subclass of ofxOceanodeNodeModel to registerType");
        
        auto &node = createNode(std::make_unique<ModelType>(), -1, true);
        node.getNodeGui().setTransformationMatrix(&transformationMatrix);
        return node;
    }
    
//    void createConnection(ofAbstractParameter& p);
    
    ofxOceanodeAbstractConnection* createConnection(ofAbstractParameter& p, ofxOceanodeNode& n);
    
    ofxOceanodeAbstractConnection* disconnectConnection(ofxOceanodeAbstractConnection* c);
    void destroyConnection(ofxOceanodeAbstractConnection* c);
    
    ofAbstractParameter& getTemporalConnectionParameter(){return temporalConnection->getSourceParameter();};
    void destroyTemporalConnection(){temporalConnectionDestructor();};//Make some checks?
    
    bool isOpenConnection(){return temporalConnection != nullptr;}
    
    template<typename Tsource, typename Tsink>
    ofxOceanodeAbstractConnection* connectConnection(ofParameter<Tsource>& source, ofParameter<Tsink>& sink){
        connections.push_back(make_pair(temporalConnectionNode, make_shared<ofxOceanodeConnection<Tsource, Tsink>>(source, sink)));
        temporalConnectionNode->addOutputConnection(connections.back().second.get());
        temporalConnectionDestructor();
        return connections.back().second.get();
    }
    ofxOceanodeAbstractConnection* createConnectionFromInfo(string sourceModule, string sourceParameter, string sinkModule, string sinkParameter);
    ofxOceanodeAbstractConnection* createConnectionFromCustomType(ofAbstractParameter &source, ofAbstractParameter &sink);
    
    shared_ptr<ofxOceanodeNodeRegistry> getRegistry(){return registry;};
    shared_ptr<ofxOceanodeTypesRegistry> getTypesRegistry(){return typesRegistry;};
    
    bool loadPreset(string presetFolderPath);
    void savePreset(string presetFolderPath);
    
    bool loadClipboardModulesAndConnections(glm::vec2 referencePosition);
    void saveClipboardModulesAndConnections(vector<ofxOceanodeNode*> nodes, glm::vec2 referencePosition);
    
    void savePersistent();
    void loadPersistent();
    void updatePersistent();
    void saveCurrentPreset();
    
    void setBpm(float _bpm);
    void setPhase(float _phase);
    void resetPhase();
    
    ofEvent<string> loadPresetEvent;
    ofEvent<float> changedBpmEvent;
    ofEvent<void> saveCurrentPresetEvent;
    ofEvent<ofxOceanodeNode*> newNodeCreated;
    
#ifdef OFXOCEANODE_USE_OSC
    void setupOscSender(string host, int port);
    void setupOscReceiver(int port);
    void receiveOsc();
    void receiveOscMessage(ofxOscMessage &m);
#endif
    
#ifdef OFXOCEANODE_USE_MIDI
    void setIsListeningMidi(bool b);
    shared_ptr<ofxOceanodeAbstractMidiBinding> createMidiBinding(ofAbstractParameter &p, bool isPersistent = false, int _id = -1);
    bool removeLastMidiBinding(ofAbstractParameter &p);
    shared_ptr<ofxOceanodeAbstractMidiBinding> createMidiBindingFromInfo(string module, string parameter, bool isPersistent = false, int _id = -1);
    vector<string> getMidiDevices(){return midiInPortList;};
    void addNewMidiMessageListener(ofxMidiListener* listener);
    
    map<string, vector<shared_ptr<ofxOceanodeAbstractMidiBinding>>>& getMidiBindings(){return midiBindings;};
    map<string, vector<shared_ptr<ofxOceanodeAbstractMidiBinding>>>& getPersistentMidiBindings(){return persistentMidiBindings;};
#endif
    
    ofParameter<glm::mat4> &getTransformationMatrix(){return transformationMatrix;};
    
#ifndef OFXOCEANODE_HEADLESS
    vector<ofxOceanodeNodeGui*> getModulesGuiInRectangle(ofRectangle rect, bool entire);
    vector<ofxOceanodeNode*> getModulesInRectangle(ofRectangle rect, bool entire);
    bool copyModulesAndConnectionsInsideRect(ofRectangle rect, bool entire);
    bool cutModulesAndConnectionsInsideRect(ofRectangle rect, bool entire);
    bool pasteModulesAndConnectionsInPosition(glm::vec2 position);
    void setWindow(std::shared_ptr<ofAppBaseWindow> window);
    void setAutoUpdateAndDraw(bool b);
    
    vector<shared_ptr<ofxOceanodeAbstractConnection>> getAllConnections();
#endif
    
private:
    void temporalConnectionDestructor();
    
    //NodeModel;
    std::unordered_map<string, nodeContainerWithId> dynamicNodes;
    std::unordered_map<string, nodeContainerWithId> persistentNodes;

    string temporalConnectionTypeName;
    ofxOceanodeNode* temporalConnectionNode;
    ofxOceanodeTemporalConnection*   temporalConnection;
    vector<pair<ofxOceanodeNode*, shared_ptr<ofxOceanodeAbstractConnection>>> connections;
    std::shared_ptr<ofxOceanodeNodeRegistry>   registry;
    std::shared_ptr<ofxOceanodeTypesRegistry>   typesRegistry;
    
    ofEventListeners destroyNodeListeners;
    ofEventListeners duplicateNodeListeners;
    ofEventListeners destroyConnectionListeners;
    
    ofEventListener updateListener;
    ofEventListener drawListener;
    bool autoUpdateAndDraw;
    
    shared_ptr<ofAppBaseWindow> window;
    
    ofParameter<glm::mat4> transformationMatrix;
    float bpm;
    float phase;
    
#ifdef OFXOCEANODE_USE_OSC
    ofxOscSender oscSender;
    ofxOscReceiver oscReceiver;
#endif
    
#ifdef OFXOCEANODE_USE_MIDI
    bool isListeningMidi;
    map<string, vector<shared_ptr<ofxOceanodeAbstractMidiBinding>>> midiBindings;
    map<string, vector<shared_ptr<ofxOceanodeAbstractMidiBinding>>> persistentMidiBindings;
    map<string, ofxMidiIn> midiIns;
    map<string, ofxMidiOut> midiOuts;
    
    vector<string> midiInPortList;
    vector<string> midiOutPortList;
    
    ofEventListeners midiUnregisterlisteners;
    ofEventListeners midiSenderListeners;
    void midiBindingBound(const void * sender, string &portName);
#endif
    
};

#endif /* ofxOceanodeContainer_h */
