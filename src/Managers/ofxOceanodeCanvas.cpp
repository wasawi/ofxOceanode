//
//  ofxOceanode.cpp
//  example-basic
//
//  Created by Eduard Frigola on 19/06/2017.
//
//

#ifndef OFXOCEANODE_HEADLESS

#include "ofxOceanodeCanvas.h"
#include "ofxOceanodeNodeRegistry.h"
#include "ofxOceanodeContainer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

void ofxOceanodeCanvas::setup(std::shared_ptr<ofAppBaseWindow> window){    
    listeners.push(window->events().update.newListener(this, &ofxOceanodeCanvas::update));
    listeners.push(window->events().draw.newListener(this, &ofxOceanodeCanvas::draw));
    
    listeners.push(window->events().mouseDragged.newListener(this,&ofxOceanodeCanvas::mouseDragged));
    listeners.push(window->events().mouseMoved.newListener(this,&ofxOceanodeCanvas::mouseMoved));
    listeners.push(window->events().mousePressed.newListener(this,&ofxOceanodeCanvas::mousePressed));
    listeners.push(window->events().mouseReleased.newListener(this,&ofxOceanodeCanvas::mouseReleased));
    listeners.push(window->events().mouseScrolled.newListener(this,&ofxOceanodeCanvas::mouseScrolled));
    listeners.push(window->events().mouseEntered.newListener(this,&ofxOceanodeCanvas::mouseEntered));
    listeners.push(window->events().mouseExited.newListener(this,&ofxOceanodeCanvas::mouseExited));
    listeners.push(window->events().keyPressed.newListener(this, &ofxOceanodeCanvas::keyPressed));
    listeners.push(window->events().keyReleased.newListener(this, &ofxOceanodeCanvas::keyReleased));
    
    transformationMatrix = &container->getTransformationMatrix();
    
    ofxDatGui::setAssetPath("");
    
    ///POP UP MENuS
    popUpMenu = new ofxDatGui(-1, -1, window);
    popUpMenu->setVisible(false);
    searchField = popUpMenu->addTextInput("Search: ");
    searchField->setNotifyEachChange(true);
    auto const &models = container->getRegistry()->getRegisteredModels();
    auto const &categories = container->getRegistry()->getCategories();
    auto const &categoriesModelsAssociation = container->getRegistry()->getRegisteredModelsCategoryAssociation();
    
    vector<string> categoriesVector;
    for(auto cat : categories){
        categoriesVector.push_back(cat);
    }
    
    vector<vector<string>> options = vector<vector<string>>(categories.size());
    for(int i = 0; i < categories.size(); i++){
        vector<string> options;
        for(auto &model : models){
            if(categoriesModelsAssociation.at(model.first) == categoriesVector[i]){
                options.push_back(model.first);
            }
        }
        std::sort(options.begin(), options.end());
        modulesSelectors.push_back(popUpMenu->addDropdown(categoriesVector[i], options));
        //modulesSelectors.back()->expand();
    }
    
    std::unique_ptr<ofxDatGuiTheme> theme = make_unique<ofxDatGuiThemeCharcoal>();
    //    theme->color.textInput.text = ;
    //    theme->color.icons = color;
    theme->layout.width = 290;
    popUpMenu->setTheme(theme.get(), true);
    searchField->setLabelColor(theme->color.label*2);
    for(auto drop : modulesSelectors){
        drop->setLabelColor(theme->color.label*2);
    }
    
    popUpMenu->onDropdownEvent(this, &ofxOceanodeCanvas::newModuleListener);
    popUpMenu->onTextInputEvent(this, &ofxOceanodeCanvas::searchListener);
    selectedRect = ofRectangle(0, 0, 0, 0);
    dragModulesInitialPoint = glm::vec2(NAN, NAN);
    selecting = false;
    
    gui.setup(nullptr, false);
}

void ofxOceanodeCanvas::draw(ofEventArgs &args){
    if(selecting){
        ofPushStyle();
        ofSetColor(255, 255, 255, 40);
        ofDrawRectangle(ofRectangle(canvasToScreen(selectInitialPoint), canvasToScreen(selectEndPoint)));
        ofPopStyle();
    }
    else if(selectedRect != ofRectangle(0,0,0,0)){
        ofPushStyle();
        if(entireSelect)
            ofSetColor(255, 80, 0, 40);
        else
            ofSetColor(0, 80, 255, 40);
        ofDrawRectangle(ofRectangle(canvasToScreen(selectedRect.getTopLeft()), canvasToScreen(selectedRect.getBottomRight())));
        ofPopStyle();
    }
    
    //Draw Guis
    gui.begin();
    
    // Draw a list of nodes on the left side
    bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;
//    ImGui::BeginChild("node_list", ImVec2(100, 0));
//    ImGui::Text("Nodes");
//    ImGui::Separator();
//    for (int node_idx = 0; node_idx < nodes.Size; node_idx++)
//    {
//        Node* node = &nodes[node_idx];
//        ImGui::PushID(node->ID);
//        if (ImGui::Selectable(node->Name, node->ID == node_selected))
//            node_selected = node->ID;
//        if (ImGui::IsItemHovered())
//        {
//            node_hovered_in_list = node->ID;
//            open_context_menu |= ImGui::IsMouseClicked(1);
//        }
//        ImGui::PopID();
//    }
//    ImGui::EndChild();
    
    ImGui::SameLine();
    ImGui::BeginGroup();
    
    const float NODE_SLOT_RADIUS = 4.0f;
    const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);
    
    // Create our child canvas
    ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Checkbox("Show grid", &show_grid);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
    ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(120.0f);
    
    ImVec2 offset = /*ImGui::GetCursorScreenPos()*/ + scrolling;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // Display grid
    if (show_grid)
    {
        ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
        float GRID_SZ = 64.0f;
        ImVec2 win_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetWindowSize();
        for (float x = fmodf(scrolling.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
            draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
        for (float y = fmodf(scrolling.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
            draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
    }
    
    auto getSourceConnectionPositionFromParameter = [this](ofAbstractParameter& param) -> glm::vec2{
        for(auto &node : container->getModulesGuiInRectangle(ofGetWindowRect(), false)){
            if(node->getParameters()->getEscapedName()  == param.getGroupHierarchyNames()[0]){
                return node->getSourceConnectionPositionFromParameter(param);
            }
        }
    };
    auto getSinkConnectionPositionFromParameter = [this](ofAbstractParameter& param) -> glm::vec2{
        for(auto &node : container->getModulesGuiInRectangle(ofGetWindowRect(), false)){
            if(node->getParameters()->getEscapedName() == param.getGroupHierarchyNames()[0]){
                return node->getSinkConnectionPositionFromParameter(param);
            }
        }
    };
    
    // Display links
    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(0); // Background
    for(auto &connection : container->getAllConnections()){
        glm::vec2 p1 = getSourceConnectionPositionFromParameter(connection->getSourceParameter());
        glm::vec2 p2 = getSinkConnectionPositionFromParameter(connection->getSinkParameter());
        draw_list->AddBezierCurve(p1, p1 + ImVec2(+100, 0), p2 + ImVec2(-100, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
    }
    if(container->isOpenConnection()){
        if(!ImGui::IsMouseDown(1)){
            container->destroyTemporalConnection();
        }else{
            glm::vec2 p1 = getSourceConnectionPositionFromParameter(container->getTemporalConnectionParameter());
            glm::vec2 p2 = ImGui::GetMousePos();
            draw_list->AddBezierCurve(p1, p1 + ImVec2(+100, 0), p2 + ImVec2(-100, 0), p2, IM_COL32(200, 200, 100, 255), 3.0f);
        }
    }
    
    // Display nodes
    int nodeId = -1;
    for(auto &node : container->getModulesGuiInRectangle(ofGetWindowRect(), false)){
        nodeId++;
        ImGui::PushID(node->getParameters()->getName().c_str());
        
        glm::vec2 node_rect_min = offset + node->getPosition();
        
        // Display node contents first
        draw_list->ChannelsSetCurrent(1); // Foreground
        bool old_any_active = ImGui::IsAnyItemActive();
        ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
        
        //Draw Parameters
        node->constructGui();
        
        // Save the size of what we have emitted and whether any of the widgets are being used
        bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
        glm::vec2 size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
        ImVec2 node_rect_max = node_rect_min + size;
        
        
        
        // Display node box
        draw_list->ChannelsSetCurrent(0); // Background
        ImGui::SetCursorScreenPos(node_rect_min);
        ImGui::InvisibleButton("node", size);
        
        if (ImGui::IsItemHovered())
        {
            node_hovered_in_scene = nodeId;
            open_context_menu |= ImGui::IsMouseClicked(1);
        }
        bool node_moving_active = ImGui::IsItemActive();
        if (node_widgets_active || node_moving_active)
            node_selected = nodeId;
        if (node_moving_active && ImGui::IsMouseDragging(0))
            node->setPosition(node->getPosition() + ImGui::GetIO().MouseDelta);
        
        
        
        ImU32 node_bg_color = /*(node_hovered_in_list == node->ID || node_hovered_in_scene == node->ID || (node_hovered_in_list == -1 && node_selected == node->ID)) ? IM_COL32(75, 75, 75, 255) :*/ IM_COL32(60, 60, 60, 255);
        draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
        draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 4.0f);
        
//        auto GetInputSlotPos = [node_rect_min, size, node](int idx) -> glm::vec2{
//            return glm::vec2(node_rect_min.x, node_rect_min.y + size.y * ((float)idx + 2) / ((float)node->getParameters()->size() + 2));
//        };
//
//        auto GetOutputSlotPos = [node_rect_min, size, node](int idx) -> glm::vec2{
//            return glm::vec2(node_rect_min.x + size.x, node_rect_min.y + size.y * ((float)idx + 2) / ((float)node->getParameters()->size() + 2));
//        };
        
//        for (int slot_idx = 0; slot_idx < node->getParameters()->size(); slot_idx++)
//            draw_list->AddCircleFilled(offset + GetInputSlotPos(slot_idx), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
//        for (int slot_idx = 0; slot_idx < node->getParameters()->size(); slot_idx++)
//            draw_list->AddCircleFilled(offset + GetOutputSlotPos(slot_idx), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
        
        for (auto &param : *node->getParameters().get())
            draw_list->AddCircleFilled(node->getSinkConnectionPositionFromParameter(*param) - glm::vec2(NODE_WINDOW_PADDING.x, 0), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
        for (auto &param : *node->getParameters().get())
            draw_list->AddCircleFilled(node->getSourceConnectionPositionFromParameter(*param) + glm::vec2(NODE_WINDOW_PADDING.x, 0), NODE_SLOT_RADIUS, IM_COL32(150, 150, 150, 150));
        
        ImGui::PopID();
    }
    
    draw_list->ChannelsMerge();
    
    // Open context menu
//    if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1))
//    {
//        node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
//        open_context_menu = true;
//    }
//    if (open_context_menu)
//    {
//        ImGui::OpenPopup("context_menu");
//        if (node_hovered_in_list != -1)
//            node_selected = node_hovered_in_list;
//        if (node_hovered_in_scene != -1)
//            node_selected = node_hovered_in_scene;
//    }
    
    // Draw context menu
//    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
//    if (ImGui::BeginPopup("context_menu"))
//    {
//        Node* node = node_selected != -1 ? &nodes[node_selected] : NULL;
//        ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
//        if (node)
//        {
//            ImGui::Text("Node '%s'", node->Name);
//            ImGui::Separator();
//            if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
//            if (ImGui::MenuItem("Delete", NULL, false, false)) {}
//            if (ImGui::MenuItem("Copy", NULL, false, false)) {}
//        }
//        else
//        {
//            if (ImGui::MenuItem("Add")) { nodes.push_back(Node(nodes.Size, "New node", scene_pos, 0.5f, ImColor(100, 100, 200), 2, 2)); }
//            if (ImGui::MenuItem("Paste", NULL, false, false)) {}
//        }
//        ImGui::EndPopup();
//    }
//    ImGui::PopStyleVar();
    
    // Scrolling
    if (ImGui::IsWindowHovered() /*&& !ImGui::IsAnyItemActive() */&& ImGui::IsMouseDragging(0, 0.0f)){
        scrolling = scrolling + ImGui::GetIO().MouseDelta;
    }
    
    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();
    
    gui.end();
    
    ofPushMatrix();
    ofMultMatrix(glm::inverse(transformationMatrix->get()));
    gui.draw();
    ofPopMatrix();
}

void ofxOceanodeCanvas::newModuleListener(ofxDatGuiDropdownEvent e){
    unique_ptr<ofxOceanodeNodeModel> type = container->getRegistry()->create(e.target->getChildAt(e.child)->getName());
    
    if (type)
    {
        auto &node = container->createNode(std::move(type));
        
        node.getNodeGui().setPosition(screenToCanvas(glm::vec2(popUpMenu->getPosition().x, popUpMenu->getPosition().y)));
        node.getNodeGui().setTransformationMatrix(transformationMatrix);
    }
    popUpMenu->setVisible(false);
    searchField->setFocused(false);
    popUpMenu->setPosition(-1, -1);
    for(auto drop : modulesSelectors){
        drop->setLabel(drop->getName());
        drop->collapse();
        for(int i = 0; i < drop->getNumOptions(); i++){
            drop->getChildAt(i)->setVisible(true);
        }
    }
}

void ofxOceanodeCanvas::searchListener(ofxDatGuiTextInputEvent e){
    searchedOptions.clear();
    if(e.text != ""){
        for(auto drop : modulesSelectors){
            int numMathes = 0;
            for(int i = 0; i < drop->getNumOptions(); i++){
                auto item = drop->getChildAt(i);
                string lowercaseName = item->getName();
                std::transform(lowercaseName.begin(), lowercaseName.end(), lowercaseName.begin(), ::tolower);
                if(ofStringTimesInString(item->getName(), e.text) || ofStringTimesInString(lowercaseName, e.text)){
                    searchedOptions.push_back(make_pair(drop, i));
                    numMathes++;
                }else{
                    item->setVisible(false);
                }
            }
            if(numMathes == 0){
                for(int i = 0; i < drop->getNumOptions(); i++){
                    drop->getChildAt(i)->setVisible(true);
                }
                drop->collapse();
            }else{
                drop->expand();
            }
        }
        if(searchedOptions.size() == 0){
            for(auto drop : modulesSelectors){
                drop->collapse();
                for(int i = 0; i < drop->getNumOptions(); i++){
                    drop->getChildAt(i)->setVisible(true);
                }
            }
        }
    }else{
        for(auto drop : modulesSelectors){
            drop->setLabel(drop->getName());
            drop->collapse();
            for(int i = 0; i < drop->getNumOptions(); i++){
                drop->getChildAt(i)->setVisible(true);
            }
        }
    }
}

void ofxOceanodeCanvas::keyPressed(ofKeyEventArgs &e){
    if(searchField->ofxDatGuiComponent::getFocused()){
        if(e.key == OF_KEY_RETURN){
            if(searchedOptions.size() != 0){
                ofxDatGuiDropdownEvent e(searchedOptions[0].first, 0, searchedOptions[0].second);
                newModuleListener(e);
            }
        }
    }else{
        if(e.key == ' '){
            //glfwSetCursor((GLFWwindow*)ofGetWindowPtr()->getWindowContext(), openedHandCursor);
            if(ofGetMousePressed()) dragCanvasInitialPoint = glm::vec2(ofGetMouseX(), ofGetMouseY());
        }else if(e.key == OF_KEY_BACKSPACE){
            popUpMenu->setVisible(false);
            searchField->setFocused(false);
            toMoveNodes.clear();
            selectedRect = ofRectangle(0,0,0,0);
        }
#ifdef TARGET_OSX
        else if(ofGetKeyPressed(OF_KEY_COMMAND)){
#else
        else if(ofGetKeyPressed(OF_KEY_CONTROL)){
#endif
            if(e.key == 'c' || e.key == 'C'){
                container->copyModulesAndConnectionsInsideRect(selectedRect, entireSelect);
                toMoveNodes.clear();
                selectedRect = ofRectangle(0,0,0,0);
            }else if(e.key == 'v' || e.key == 'V'){
                container->pasteModulesAndConnectionsInPosition(screenToCanvas(glm::vec2(ofGetMouseX(), ofGetMouseY())));
            }else if(e.key == 'x' || e.key == 'X'){
                container->cutModulesAndConnectionsInsideRect(selectedRect, entireSelect);
                toMoveNodes.clear();
                selectedRect = ofRectangle(0,0,0,0);
            }
        }
    }
}

void ofxOceanodeCanvas::mouseDragged(ofMouseEventArgs &e){
    glm::vec2 transformedPos = screenToCanvas(e);
    if(ofGetKeyPressed(' ')){
        transformationMatrix->set(glm::translate(transformationMatrix->get(), glm::vec3(dragCanvasInitialPoint-e, 0)));
        dragCanvasInitialPoint = e;
    }else if(selecting){
        selectEndPoint = transformedPos;
    }else if(toMoveNodes.size() != 0 && dragModulesInitialPoint == dragModulesInitialPoint){
        for(auto node : toMoveNodes){
            node.first->setPosition(node.second + (transformedPos - dragModulesInitialPoint));
        }
        selectedRect.setPosition(glm::vec3(selectedRectIntialPosition + (transformedPos - dragModulesInitialPoint), 1));
    }
}

void ofxOceanodeCanvas::mousePressed(ofMouseEventArgs &e){
    glm::vec2 transformedPos = screenToCanvas(e);
#ifdef TARGET_OSX
    if(ofGetKeyPressed(OF_KEY_COMMAND)){
#else
    if(ofGetKeyPressed(OF_KEY_CONTROL)){
#endif
        if(e.button == 0){
            searchField->setText("");
            searchField->setFocused(true);
            popUpMenu->setPosition(e.x, e.y);
            popUpMenu->setVisible(true);
        }
        else if(e.button == 2){
            transformationMatrix->set(glm::mat4(1.0));
        }
    }
    if(ofGetKeyPressed(' ')){
        dragCanvasInitialPoint = e;
    }
    if(ofGetKeyPressed(OF_KEY_ALT)){
        selectInitialPoint = transformedPos;
        selectEndPoint = transformedPos;
        selectedRect = ofRectangle(0,0,0,0);
        selecting = true;
    }
    if(toMoveNodes.size() != 0 && selectedRect.inside(transformedPos)){
        selectedRectIntialPosition = selectedRect.getPosition();
        for(auto &node : toMoveNodes){
            node.second = node.first->getPosition();
        }
        dragModulesInitialPoint = transformedPos;
    }
}

void ofxOceanodeCanvas::mouseReleased(ofMouseEventArgs &e){
    if(selecting){
        selectedRect = ofRectangle(selectInitialPoint, selectEndPoint);
        if(glm::all(glm::greaterThan(selectInitialPoint, selectEndPoint)))
            entireSelect = false;
        else
            entireSelect = true;
        
        toMoveNodes.clear();
        for(auto nodeGui : container->getModulesGuiInRectangle(selectedRect, entireSelect)){
            toMoveNodes.push_back(make_pair(nodeGui, nodeGui->getPosition()));
        }
        if(toMoveNodes.size() == 0){
            selectedRect = ofRectangle(0,0,0,0);
        }
    }
    dragModulesInitialPoint = glm::vec2(NAN, NAN);
    selecting = false;
}

void ofxOceanodeCanvas::mouseScrolled(ofMouseEventArgs &e){
#ifdef TARGET_OSX
    if(ofGetKeyPressed(OF_KEY_COMMAND)){
#else
    if(ofGetKeyPressed(OF_KEY_CONTROL)){
#endif
        float scrollValue = -e.scrollY/100.0;
        transformationMatrix->set(translateMatrixWithoutScale(transformationMatrix->get(), glm::vec3(e, 0) * getMatrixScale(transformationMatrix->get()) * scrollValue));
        transformationMatrix->set(glm::scale(transformationMatrix->get(), glm::vec3(1-(scrollValue), 1-(scrollValue), 1)));
    }else if(ofGetKeyPressed(OF_KEY_ALT)){
        transformationMatrix->set(translateMatrixWithoutScale(transformationMatrix->get(), glm::vec3(e.scrollY*2, 0, 0)));
    }else{
        transformationMatrix->set(translateMatrixWithoutScale(transformationMatrix->get(), glm::vec3(-e.scrollX*2, -e.scrollY*2, 0)));
    }
}

glm::vec2 ofxOceanodeCanvas::screenToCanvas(glm::vec2 p){
    glm::vec4 result = transformationMatrix->get() * glm::vec4(p, 0, 1);
    return result;
}

glm::vec2 ofxOceanodeCanvas::canvasToScreen(glm::vec2 p){
    glm::vec4 result = glm::inverse(transformationMatrix->get()) * glm::vec4(p, 0, 1);
    return result;
}

glm::vec3 ofxOceanodeCanvas::getMatrixScale(const glm::mat4 &m){
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(m, scale, rotation, translation, skew, perspective);
    return scale;
}

glm::mat4 ofxOceanodeCanvas::translateMatrixWithoutScale(const glm::mat4 &m, glm::vec3 translationVector){
    return glm::translate(glm::mat4(1.0), translationVector) * m;
}

#endif
