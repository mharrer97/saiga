/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "sample.h"

#include "saiga/core/image/imageTransformations.h"
#include "saiga/core/math/random.h"
#include "saiga/core/util/color.h"

#include <eigen3/Eigen/Core>
#include <saiga/core/imgui/imgui.h>

#if defined(SAIGA_OPENGL_INCLUDED)
#    error OpenGL was included somewhere.
#endif

VulkanExample::~VulkanExample()
{
    LOG(INFO) << "~VulkanExample";
    assetRenderer.destroy();
    lineAssetRenderer.destroy();
    pointCloudRenderer.destroy();
    texturedAssetRenderer.destroy();
}

VulkanExample::VulkanExample()
{
    auto& base = renderer->base();


    assetRenderer.deferred.init(base, renderer->renderPass);
    assetRenderer.forward.init(base, renderer->forwardPass);
    assetRenderer.shadow.init(base, renderer->lighting.shadowPass);
    lineAssetRenderer.deferred.init(base, renderer->renderPass, 2);
    lineAssetRenderer.forward.init(base, renderer->forwardPass, 2);
    lineAssetRenderer.shadow.init(base, renderer->lighting.shadowPass, 2);
    pointCloudRenderer.deferred.init(base, renderer->renderPass, 5);
    pointCloudRenderer.forward.init(base, renderer->forwardPass, 5);
    pointCloudRenderer.shadow.init(base, renderer->lighting.shadowPass, 5);
    texturedAssetRenderer.deferred.init(base, renderer->renderPass);
    texturedAssetRenderer.forward.init(base, renderer->forwardPass);
    texturedAssetRenderer.shadow.init(base, renderer->lighting.shadowPass);

    cornell.loadObj("Cornell.obj");
    cornell.init(renderer->base());

    sphere.loadObj("icosphere.obj");
    sphere.init(renderer->base());

    spotLight = renderer->lighting.createSpotLight();
    // spotLight->setColorDiffuse(Saiga::Vulkan::Lighting::LightColorPresets::Candle);
    // spotLight->setColorSpecular(Saiga::Vulkan::Lighting::LightColorPresets::Candle);
    spotLight->setAttenuation(Saiga::Vulkan::Lighting::AttenuationPresets::Quadratic);
    spotLight->setIntensity(1);
    spotLight->setRadius(3);
    spotLight->setPosition(vec3(0, 18.f - 10.f, 0));
    spotLight->setDirection(vec3(0.f, -1.f, 0.f));
    spotLight->setColorDiffuse(make_vec3(1));
    spotLight->setColorSpecular(make_vec3(1));
    spotLight->calculateModel();
    renderer->lighting.enableShadowMapping(spotLight, 4000);

    directionalLight = renderer->lighting.createDirectionalLight();
    directionalLight->setColorDiffuse(make_vec3(1));
    directionalLight->setAmbientIntensity(0.1f);
    directionalLight->setIntensity(0.f);

    float aspect = window->getAspectRatio();
    camera.setProj(60.0f, aspect, 0.1f, 150.0f, true);
    camera.setView(vec3(0, 0, 27.5), vec3(0, 0, 0), vec3(0, 1, 0));
    camera.rotationPoint = make_vec3(0);
}



void VulkanExample::update(float dt)
{
    if (!ImGui::captureKeyboard())
    {
        camera.update(dt);
    }
    if (!ImGui::captureMouse())
    {
        camera.interpolate(dt, 0);
    }

    spotLight->setPosition(vec3(0, lightHeight - 10.f, 0));
    spotLight->setRadius(lightRadius);
    spotLight->setAngle(spotLightOpeningAngle);
    spotLight->calculateModel();

    directionalLight->setAmbientIntensity(ambientIntensity);
}

void VulkanExample::transfer(vk::CommandBuffer cmd, Camera* cam)
{
    assetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    // pointCloudRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    // lineAssetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    // texturedAssetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
}

void VulkanExample::transferDepth(vk::CommandBuffer cmd, Camera* cam)
{
    assetRenderer.shadow.updateUniformBuffers(cmd, cam->view, cam->proj);
    // lineAssetRenderer.shadow.updateUniformBuffers(cmd, cam->view, cam->proj);
    // pointCloudRenderer.shadow.updateUniformBuffers(cmd, cam->view, cam->proj);
    // texturedAssetRenderer.shadow.updateUniformBuffers(cmd, cam->view, cam->proj);
}

void VulkanExample::transferForward(vk::CommandBuffer cmd, Camera* cam)
{
    assetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    // pointCloudRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    // lineAssetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    // texturedAssetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
}

void VulkanExample::render(vk::CommandBuffer cmd, Camera* cam)
{
    if (displayModels)
    {
        if (assetRenderer.deferred.bind(cmd))
        {
            assetRenderer.deferred.pushModel(cmd, translate(vec3(0.f, -10.f, 0.f)) * scale(make_vec3(10.f)));
            cornell.render(cmd);
        }
    }
}

void VulkanExample::renderDepth(vk::CommandBuffer cmd, Camera* cam)
{
    if (displayModels)
    {
        if (assetRenderer.shadow.bind(cmd))
        {
            assetRenderer.shadow.pushModel(cmd, translate(vec3(0.f, -10.f, 0.f)) * scale(make_vec3(10.f)));
            cornell.render(cmd);
        }
    }
}

void VulkanExample::renderForward(vk::CommandBuffer cmd, Camera* cam)
{
    if (displayModels)
    {
        if (assetRenderer.forward.bind(cmd))
        {
            assetRenderer.shadow.pushModel(cmd, translate(spotLight->getPosition()) * scale(make_vec3(0.1f)));
            sphere.render(cmd);
        }
    }
}
void VulkanExample::renderGUI()
{
    ImGui::SetNextWindowSize(ImVec2(350, 250));
    ImGui::Begin("Example settings");
    ImGui::Checkbox("Render models", &displayModels);


    if (ImGui::Button("reload shader"))
    {
        //        texturedAssetRenderer.shaderPipeline.reload();
        texturedAssetRenderer.reload();
        assetRenderer.reload();
        lineAssetRenderer.reload();
        pointCloudRenderer.reload();
        renderer->reload();
    }

    ImGui::DragFloat("Light Radius", &lightRadius, 0.05f, 0.f, 55.f);
    ImGui::DragFloat("Opening Angle", &spotLightOpeningAngle, 0.25f, 0.f, 360.f);
    ImGui::DragFloat("Light Height", &lightHeight, 0.05f, 0.f, 20.f);
    ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.005f, 0.f, 1.f);
    ImGui::End();
    //    return;

    window->renderImGui();
    //    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
    //    ImGui::ShowTestWindow();
}


#undef main

int main(const int argc, const char* argv[])
{
    using namespace Saiga;

    {
        VulkanExample example;

        example.run();
    }

    return 0;
}
