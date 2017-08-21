/**
 * Copyright (c) 2017 Darius Rückert 
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "volumetricLights.h"

#include "saiga/rendering/deferred_renderer.h"
#include "saiga/rendering/lighting/directional_light.h"
#include "saiga/opengl/shader/shaderLoader.h"

#include "saiga/geometry/triangle_mesh_generator.h"

VolumetricLights::VolumetricLights(OpenGLWindow *window): Program(window),
   tdo(window->getWidth(),window->getHeight())
{
    //this simplifies shader debugging
//    ShaderLoader::instance()->addLineDirectives = true;

    parentWindow->showRendererImgui = true;
    parentWindow->getRenderer()->showLightingImgui = true;

    //create a perspective camera
    float aspect = window->getAspectRatio();
    camera.setProj(60.0f,aspect,0.1f,75.0f);
    auto pos = vec3(23.7,12.5,-6.1);
    auto dir = -vec3(0.617,0.0656,-0.784);
    camera.setView(pos,pos+dir,vec3(0,1,0));
    camera.enableInput();
    //How fast the camera moves
    camera.movementSpeed = 10;
    camera.movementSpeedFast = 20;

    //Set the camera from which view the scene is rendered
    window->setCamera(&camera);


    //add this object to the keylistener, so keyPressed and keyReleased will be called
    SDL_EventHandler::addKeyListener(this);

    ObjAssetLoader assetLoader;


    auto cubeAsset = assetLoader.loadTexturedAsset("objs/box.obj");
    auto cubeAsset2 = assetLoader.loadTexturedAsset("objs/sponza.obj");

    cube1.asset = cubeAsset;
    cube2.asset = cubeAsset2;
    cube1.translateGlobal(vec3(11,1,-2));
    cube1.calculateModel();

    cube2.setScale(vec3(0.02));
//    cube2.translateGlobal(vec3(-11,1,2));
    cube2.calculateModel();

    auto sphereAsset = assetLoader.loadBasicAsset("objs/teapot.obj");
    sphere.asset = sphereAsset;
    sphere.translateGlobal(vec3(0,1,8));
    sphere.rotateLocal(vec3(0,1,0),180);
    sphere.calculateModel();

    groundPlane.asset = assetLoader.loadDebugPlaneAsset(vec2(20,20),1.0f,Colors::lightgray,Colors::gray);


    ShadowQuality sq = ShadowQuality::HIGH;

    sun = window->getRenderer()->lighting.createDirectionalLight();
    sun->setDirection(vec3(2,-3,0.4));
    sun->setColorDiffuse(LightColorPresets::DirectSunlight);
    sun->setIntensity(0.3);
    sun->setAmbientIntensity(0.1f);
    sun->createShadowMap(2048,2048,1,sq);
//    sun->enableShadows();
//    sun->setActive(false);

        pointLight = window->getRenderer()->lighting.createPointLight();
        pointLight->setAttenuation(AttenuationPresets::Quadratic);
        pointLight->setIntensity(2);
        pointLight->setRadius(10);
        pointLight->setPosition(vec3(10,3,0));
        pointLight->setColorDiffuse(vec3(1));
        pointLight->calculateModel();
        pointLight->createShadowMap(256,256,sq);
        pointLight->enableShadows();
        pointLight->setActive(false);

        spotLight = window->getRenderer()->lighting.createSpotLight();
        spotLight->setAttenuation(AttenuationPresets::Quadratic);
        spotLight->setIntensity(2);
        spotLight->setRadius(8);
        spotLight->setPosition(vec3(-10,5,0));
        spotLight->setColorDiffuse(vec3(1));
        spotLight->calculateModel();
        spotLight->createShadowMap(512,512,sq);
        spotLight->enableShadows();
        spotLight->setActive(false);

        boxLight = window->getRenderer()->lighting.createBoxLight();
        boxLight->setIntensity(1.0);

//        boxLight->setPosition(vec3(0,2,10));
//        boxLight->rotateLocal(vec3(1,0,0),30);
        pos = vec3(0,19,0);
        boxLight->setView(pos,pos+vec3(3,-3,0.3),vec3(0,1,0));
        boxLight->setColorDiffuse(LightColorPresets::Tungsten40W);
        boxLight->setScale(vec3(8,20,32));
        boxLight->calculateModel();
        boxLight->createShadowMap(512,512,sq);
        boxLight->enableShadows();
//        boxLight->setActive(false);



    textAtlas.loadFont("fonts/SourceSansPro-Regular.ttf",40,2,4,true);

    tdo.init(&textAtlas);
    tdo.borderX = 0.01f;
    tdo.borderY = 0.01f;
    tdo.paddingY = 0.000f;
    tdo.textSize = 0.04f;

    tdo.textParameters.setColor(vec4(1),0.1f);
    tdo.textParameters.setGlow(vec4(0,0,0,1),1.0f);

    tdo.createItem("Fps: ");
    tdo.createItem("Ups: ");
    tdo.createItem("Render Time: ");
    tdo.createItem("Update Time: ");


    cout<<"Program Initialized!"<<endl;
}

VolumetricLights::~VolumetricLights()
{
    //We don't need to delete anything here, because objects obtained from saiga are wrapped in smart pointers.
}

void VolumetricLights::update(float dt){
    //Update the camera position
    camera.update(dt);


    sun->fitShadowToCamera(&camera);
//    sun->fitNearPlaneToScene(sceneBB);

    int  fps = (int) glm::round(1000.0/parentWindow->fpsTimer.getTimeMS());
    tdo.updateEntry(0,fps);

    int  ups = (int) glm::round(1000.0/parentWindow->upsTimer.getTimeMS());
    tdo.updateEntry(1,ups);

    float renderTime = parentWindow->getRenderer()->getTime(Deferred_Renderer::TOTAL);
    tdo.updateEntry(2,renderTime);

    float updateTime = parentWindow->updateTimer.getTimeMS();
    tdo.updateEntry(3,updateTime);


    //    sphere.rotateLocal(vec3(0,1,0),rotationSpeed);
    //    sphere.calculateModel();
}

void VolumetricLights::interpolate(float dt, float interpolation) {
    //Update the camera rotation. This could also be done in 'update' but
    //doing it in the interpolate step will reduce latency
    camera.interpolate(dt,interpolation);
}

void VolumetricLights::render(Camera *cam)
{
    //Render all objects from the viewpoint of 'cam'
    groundPlane.render(cam);
    cube1.render(cam);
    cube2.render(cam);
    sphere.render(cam);
}

void VolumetricLights::renderDepth(Camera *cam)
{
    //Render the depth of all objects from the viewpoint of 'cam'
    //This will be called automatically for shadow casting light sources to create shadow maps
    groundPlane.renderDepth(cam);
    cube1.renderDepth(cam);
    cube2.renderDepth(cam);
    sphere.render(cam);
}

void VolumetricLights::renderOverlay(Camera *cam)
{
    //The skybox is rendered after lighting and before post processing
//    skybox.render(cam);
}

void VolumetricLights::renderFinal(Camera *cam)
{

    //The final render path (after post processing).
    //Usually the GUI is rendered here.

    parentWindow->getRenderer()->bindCamera(&tdo.layout.cam);
    tdo.render();




    //{
    //    ImGui::SetNextWindowPos(ImVec2(50, 400), ImGuiSetCond_FirstUseEver);
    //    ImGui::SetNextWindowSize(ImVec2(400,200), ImGuiSetCond_FirstUseEver);
    //    ImGui::Begin("An Imgui Window :D");

    //    ImGui::SliderFloat("Rotation Speed",&rotationSpeed,0,10);

    //    ImGui::End();
    //}

    //parentWindow->renderImGui();



}


void VolumetricLights::keyPressed(SDL_Keysym key)
{
    switch(key.scancode){
    case SDL_SCANCODE_ESCAPE:
        parentWindow->close();
        break;
    case SDL_SCANCODE_BACKSPACE:
        parentWindow->getRenderer()->printTimings();
        break;
    case SDL_SCANCODE_R:
        ShaderLoader::instance()->reload();
        break;
    case SDL_SCANCODE_F12:
        parentWindow->screenshot("screenshot.png");
        break;
    default:
        break;
    }
}

void VolumetricLights::keyReleased(SDL_Keysym key)
{
}



