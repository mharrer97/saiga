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

/*VulkanExample::VulkanExample(Saiga::Vulkan::VulkanWindow& window, Saiga::Vulkan::VulkanDeferredRenderer& renderer)
    : Updating(), Saiga::Vulkan::VulkanDeferredRenderingInterface(renderer), renderer(renderer)
//: Updating(window), Saiga::Vulkan::VulkanDeferredRenderingInterface(renderer), renderer(renderer)
{
    float aspect = window.getAspectRatio();
    camera.setProj(60.0f, aspect, 0.1f, 100.0f, true);
    camera.setView(vec3(0, 5, 10), vec3(0, 0, 0), vec3(0, 1, 0));
    camera.rotationPoint = make_vec3(0);


    //   std::cout << camera.view <<std::endl;
    //   std::cout << camera.proj <<std::endl;
    //    exit(0);

    window.setCamera(&camera);

    init(renderer.base());
}
*/

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
    {
        auto tex = std::make_shared<Saiga::Vulkan::Texture2D>();

        Saiga::Image img("box.png");

        std::cout << "uncompressed size " << img.size() << std::endl;
        auto data = img.compress();
        std::cout << "compressed size " << data.size() << std::endl;
        img.decompress(data);
        std::cout << "test" << std::endl;

        if (img.type == Saiga::UC3)
        {
            std::cout << "adding alplha channel" << std::endl;
            Saiga::TemplatedImage<ucvec4> img2(img.height, img.width);
            std::cout << img << " " << img2 << std::endl;
            Saiga::ImageTransformation::addAlphaChannel(img.getImageView<ucvec3>(), img2.getImageView(), 255);
            tex->fromImage(base, img2);
        }
        else
        {
            std::cout << img << std::endl;
            tex->fromImage(base, img);
        }
        texture = tex;
    }


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
    textureDisplay.deferred.init(base, renderer->renderPass);
    textureDisplay.forward.init(base, renderer->forwardPass);

    textureDes    = textureDisplay.forward.createAndUpdateDescriptorSet(*texture);
    textureDesDef = textureDisplay.deferred.createAndUpdateDescriptorSet(*texture);


    box.loadObj("box.obj");

    ////    box.loadObj("cat.obj");
    box.init(renderer->base());
    box.descriptor = texturedAssetRenderer.forward.createAndUpdateDescriptorSet(*box.textures[0]);


    teapot.loadObj("teapot.obj");
    //        teapot.loadPly("dragon_10k.ply");
    //    teapot.loadPly("fr3_office.ply");
    // teapot.mesh.computePerVertexNormal();
    teapot.init(renderer->base());
    teapotTrans.setScale(vec3(2, 2, 2));
    //    teapotTrans.rotateGlobal(vec3(1, 0, 0), pi<float>());
    teapotTrans.translateGlobal(vec3(0, 2.5, 0));
    teapotTrans.calculateModel();

    bigSphere.loadObj("bigSphere.obj");
    // bigSphere.mesh.computePerVertexNormal();
    bigSphere.init(renderer->base());

    plane.createCheckerBoard(ivec2(20, 20), 1.0f, Saiga::Colors::firebrick, Saiga::Colors::gray);
    plane.init(renderer->base());

    sphere.loadObj("icosphere.obj");
    sphere.init(renderer->base());

    candle.loadObj("box.obj");
    // candle.mesh.computePerVertexNormal();
    candle.init(renderer->base());

    grid.createGrid(10, 10);
    grid.init(renderer->base());

    frustum.createFrustum(camera.proj, 2, make_vec4(1), true);
    frustum.init(renderer->base());

    pointCloud.init(base, 1000 * 1000);
    for (int i = 0; i < 1000 * 1000; ++i)
    {
        Saiga::VertexNC v;
        v.position               = make_vec4(linearRand(make_vec3(-3), make_vec3(3)), 1);
        v.color                  = make_vec4(linearRand(make_vec3(0), make_vec3(1)), 1);
        pointCloud.pointCloud[i] = v;
    }


    std::shared_ptr<Saiga::Vulkan::Lighting::PointLight> pointTestLight;
    int count = 5;
    for (int i = 0; i < count; ++i)
    {
        vec3 pos = vec3(sin((float(i) / float(count)) * 6.28f), 1.f, cos((float(i) / float(count)) * 6.28f));
        pos *= 15.f;

        // pos                           = vec3(1.f, 0.f, 1.f) * (3 * float(i));
        pos[1]         = 5.f;
        pointTestLight = renderer->lighting.createPointLight();
        pointTestLight->setPosition(pos);

        float ratio     = float(i) / float(count);
        vec3 lightColor = vec3(ratio, fmod(ratio + (1.f / 3.f), 1.f), fmod(ratio + (2.f / 3.f), 1.f));
        pointTestLight->setColorDiffuse(lightColor);
        pointTestLight->setColorSpecular(lightColor);
        pointLights.push_back(pointTestLight);
    }

    spotLight = renderer->lighting.createSpotLight();
    spotLight->setColorDiffuse(Saiga::Vulkan::Lighting::LightColorPresets::Candle);
    spotLight->setColorSpecular(Saiga::Vulkan::Lighting::LightColorPresets::Candle);
    renderer->lighting.enableShadowMapping(spotLight);

    boxLight = renderer->lighting.createBoxLight();
    boxLight->setColorDiffuse(Saiga::Vulkan::Lighting::LightColorPresets::MuzzleFlash);
    boxLight->setColorSpecular(Saiga::Vulkan::Lighting::LightColorPresets::MuzzleFlash);
    boxLight->setActive(true);

    directionalLight = renderer->lighting.createDirectionalLight();
    directionalLight->setColorDiffuse(Saiga::Vulkan::Lighting::LightColorPresets::MoonlightBlue);
    directionalLight->setColorSpecular(Saiga::Vulkan::Lighting::LightColorPresets::MoonlightBlue);

    directionalLight->setView(vec3(450.f, 450.f, 450.f), vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f));
    // directionalLight->setDirection(vec3(-1.f, -1.f, -1.f));
    //    directionalLight->setScale(vec3(7.5f, 7.5f, 25.f));
    directionalLight->setScale(vec3(30.f, 30.f, 30.f));

    directionalLight->setAmbientIntensity(0.f);
    directionalLight->setIntensity(0.f);
    directionalLight->calculateModel();

    // TODO adapt shadopmap creation handling
    renderer->lighting.enableShadowMapping(directionalLight);
    // directionalLight->calculateCamera();

    candleLight = renderer->lighting.createSpotLight();
    candleLight->setColorDiffuse(Saiga::Vulkan::Lighting::LightColorPresets::Candle);
    candleLight->setColorSpecular(Saiga::Vulkan::Lighting::LightColorPresets::Candle);

    candleLight->setActive(true);
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


    if (change)
    {
        //        renderer.waitIdle();
        //        for(int i = 0; i < 1000; ++i)
        for (auto& v : pointCloud.pointCloud)
        {
            //            Saiga::VertexNC v;
            v.position = make_vec4(linearRand(make_vec3(-3), make_vec3(3)), 1);
            v.color    = make_vec4(linearRand(make_vec3(0), make_vec3(1)), 1);
            //            pointCloud.mesh.points.push_back(v);
        }
        change        = false;
        uploadChanges = true;
    }

    // camera.setInput(!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse);

    timingLoop2 = fmod(timingLoop2 + dt, 2.f * 3.1415f);

    if (lightRotate)
    {
        timingLoop = fmod(timingLoop + 0.2f * dt, 2.f * 3.1415f);


        int count = pointLights.size();
        for (int i = 0; i < count; ++i)
        {
            vec3 pos = vec3(sin(fmod(((float(i) / float(count)) * 6.28f) - timingLoop, 2.f * 3.1415f)), 1.f,
                            cos(fmod(((float(i) / float(count)) * 6.28f) - timingLoop, 2.f * 3.1415f)));
            pos *= 15.f;

            // pos                           = vec3(1.f, 0.f, 1.f) * (3 * float(i));
            pos[1] = 5.f;
            pointLights[i]->setPosition(pos);
        }

        //        vec3 dir = vec3(0.01f * sin(fmod(timingLoop, 2.f * 3.1415f)), 1.f,
        //                        cos(fmod((0.1f * 6.28f) - timingLoop, 2.f * 3.1415f)));
    }

    int count = pointLights.size();
    for (int i = 0; i < count; ++i)
    {
        pointLights[i]->setRadius(lightRadius);
        pointLights[i]->calculateModel();
    }

    vec3 pos =
        vec3(sin(fmod(3.1415f - timingLoop, 2.f * 3.1415f)), 1.f, cos(fmod(3.1415f - timingLoop, 2.f * 3.1415f)));
    pos *= 10.f;
    spotLight->setRadius(lightRadius);
    spotLight->setDirection(-pos);


    pos[1] = 3.5f;
    spotLight->setPosition(pos);
    spotLight->setAngle(spotLightOpeningAngle);

    spotLight->calculateModel();

    pos    = -pos * 0.5f;
    pos[1] = 3.f;

    boxLight->setView(pos, make_vec3(0.f), vec3(0.f, 1.f, 0.f));
    boxLight->setScale(vec3(2.5f, 2.5f, 4.f));
    boxLight->setIntensity(0.5f);
    boxLight->calculateModel();

    // spotLight->setDirection(vec3(0.f, -1.f, 0.f));
    vec3 dir = vec3(0.03f * cos(fmod((0.8f * 6.28f) - timingLoop2, 2.f * 3.1415f)), 1.f,
                    0.03f * sin(fmod((1.5f * 6.28f) - timingLoop2, 2.f * 3.1415f)));
    candleLight->setDirection(dir);
    vec3 posc = vec3(-5.f, 1.12f + 0.01f * cos(fmod((0.3f * 6.28f) - timingLoop2, 2.f * 3.1415f)), -5.f);
    candleLight->setPosition(posc);
    float radius = 5.f + 0.1f * sin(fmod(timingLoop2 * 3.f, 2.f * 3.1415f));
    candleLight->setRadius(radius);
    float angle = 270.f + 3.f * cos(fmod(timingLoop2, 2.f * 3.1415f));
    candleLight->setAngle(angle);

    candleLight->setIntensity(1.f + 0.03f * cos(fmod(((0.9f * 6.28f) - timingLoop2) * 20.f, 2.f * 3.1415f)));
    candleLight->calculateModel();


    pos = vec3(sin(fmod(3.1415f - timingLoop, 2.f * 3.1415f)), 1.f, cos(fmod(3.1415f - timingLoop, 2.f * 3.1415f)));
    pos *= 15.f;
    // directionalLight->setView(pos, vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f));
    // directionalLight->setScale(vec3(25.f, 25.f, 25.f));
    // directionalLight->calculateModel();

    directionalLight->setIntensity(dirLightIntensity);
    directionalLight->setAmbientIntensity(dirLightAmbientIntensity);
    // directionalLight->fitShadowToCamera(&camera);
}

void VulkanExample::transfer(vk::CommandBuffer cmd, Camera* cam)
{
    assetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    pointCloudRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    lineAssetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    texturedAssetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);

    // upload everything every frame
    if (uploadChanges)
    {
        pointCloud.updateBuffer(cmd, 0, pointCloud.capacity);

        uploadChanges = false;
    }
}

void VulkanExample::transferDepth(vk::CommandBuffer cmd, Camera* cam)
{
    assetRenderer.shadow.updateUniformBuffers(cmd, cam->view, cam->proj);
    lineAssetRenderer.shadow.updateUniformBuffers(cmd, cam->view, cam->proj);
    pointCloudRenderer.shadow.updateUniformBuffers(cmd, cam->view, cam->proj);
    texturedAssetRenderer.shadow.updateUniformBuffers(cmd, cam->view, cam->proj);
}

void VulkanExample::transferForward(vk::CommandBuffer cmd, Camera* cam)
{
    assetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    pointCloudRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    lineAssetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    texturedAssetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
}

void VulkanExample::render(vk::CommandBuffer cmd, Camera* cam)
{
    if (displayModels)
    {
        if (assetRenderer.deferred.bind(cmd))
        {
            assetRenderer.deferred.pushModel(cmd, mat4::Identity());
            plane.render(cmd);

            assetRenderer.deferred.pushModel(cmd, teapotTrans.model);
            teapot.render(cmd);

            assetRenderer.deferred.pushModel(cmd, translate(vec3(-5.f, 0.5f, -5.f)) * scale(vec3(0.2f, 0.5f, 0.2f)));
            candle.render(cmd);

            assetRenderer.deferred.pushModel(cmd, translate(vec3(10.f, 3.f, 10.f)) * scale(vec3(3.f, 3.f, 3.f)));
            bigSphere.render(cmd);
        }

        /*if (pointCloudRenderer.deferred.bind(cmd))
        {
            pointCloudRenderer.deferred.pushModel(cmd, translate(vec3(10, 2.5f, 0)));
            pointCloud.render(cmd, 0, pointCloud.capacity);
        }*/

        /*if (lineAssetRenderer.deferred.bind(cmd))
        {
            lineAssetRenderer.deferred.pushModel(cmd, translate(vec3(-10.f, 1.5f, 0)));
            teapot.render(cmd);

            auto gridMatrix = rotate(0.5f * pi<float>(), vec3(1, 0, 0));
            gridMatrix      = translate(gridMatrix, vec3(0, -10, 0));
            lineAssetRenderer.deferred.pushModel(cmd, gridMatrix);
            grid.render(cmd);
        }*/

        if (texturedAssetRenderer.deferred.bind(cmd))
        {
            texturedAssetRenderer.deferred.pushModel(cmd, translate(vec3(-7.5f, 1, -5.f)));
            texturedAssetRenderer.deferred.bindTexture(cmd, box.descriptor);
            box.render(cmd);
        }

        /*if (textureDisplay.deferred.bind(cmd))
        {
            textureDisplay.deferred.renderTexture(cmd, textureDesDef, vec2(150, 10), vec2(100, 50));
        }*/
    }
}

void VulkanExample::renderDepth(vk::CommandBuffer cmd, Camera* cam)
{
    if (displayModels)
    {
        if (assetRenderer.shadow.bind(cmd))
        {
            assetRenderer.shadow.pushModel(cmd, mat4::Identity());
            plane.render(cmd);

            assetRenderer.shadow.pushModel(cmd, teapotTrans.model);
            teapot.render(cmd);

            assetRenderer.shadow.pushModel(cmd, translate(vec3(-5.f, 0.5f, -5.f)) * scale(vec3(0.2f, 0.5f, 0.2f)));
            candle.render(cmd);

            assetRenderer.deferred.pushModel(cmd, translate(vec3(10.f, 3.f, 10.f)) * scale(vec3(3.f, 3.f, 3.f)));
            bigSphere.render(cmd);
        }
        if (lineAssetRenderer.shadow.bind(cmd))
        {
            lineAssetRenderer.shadow.pushModel(cmd, translate(vec3(-10.f, 1.5f, 0)));
            teapot.render(cmd);

            auto gridMatrix = rotate(0.5f * pi<float>(), vec3(1, 0, 0));
            gridMatrix      = gridMatrix * translate(vec3(0, -10, 0));
            lineAssetRenderer.shadow.pushModel(cmd, gridMatrix);
            grid.render(cmd);
        }
        if (pointCloudRenderer.shadow.bind(cmd))
        {
            pointCloudRenderer.shadow.pushModel(cmd, translate(vec3(10, 2.5f, 0)));
            pointCloud.render(cmd, 0, pointCloud.capacity);
        }
        if (texturedAssetRenderer.shadow.bind(cmd))
        {
            texturedAssetRenderer.shadow.pushModel(cmd, translate(vec3(-7.5f, 1, -5.f)));
            box.render(cmd);
        }
    }
}

void VulkanExample::renderForward(vk::CommandBuffer cmd, Camera* cam)
{
    if (displayModels)
    {
        if (assetRenderer.forward.bind(cmd))
        {
            /*assetRenderer.forward.pushModel(cmd, identityMat4());
            // plane.render(cmd);



            // assetRenderer.forward.pushModel(cmd, teapotTrans.model * translate(vec3(5.f, 0.f, 5.f)));
            // teapot.render(cmd);
            */

            for (auto& l : pointLights)
            {
                assetRenderer.forward.pushModel(cmd, translate(l->getPosition()) * scale(vec3(0.1f, 0.1f, 0.1f)));
                sphere.render(cmd);
            }
            assetRenderer.forward.pushModel(cmd, translate(spotLight->getPosition()) * scale(vec3(0.1f, 0.1f, 0.1f)));
            sphere.render(cmd);

            assetRenderer.forward.pushModel(cmd, translate(candleLight->getPosition()) * scale(vec3(0.1f, 0.1f, 0.1f)));
            sphere.render(cmd);
        }
        if (pointCloudRenderer.forward.bind(cmd))
        {
            pointCloudRenderer.forward.pushModel(cmd, translate(vec3(10, 2.5f, 0)));
            pointCloud.render(cmd, 0, pointCloud.capacity);
        }
        if (lineAssetRenderer.forward.bind(cmd))
        {
            lineAssetRenderer.forward.pushModel(cmd, translate(vec3(-10.f, 1.5f, 0)));
            teapot.render(cmd);

            auto gridMatrix = rotate(0.5f * pi<float>(), vec3(1, 0, 0));
            gridMatrix      = gridMatrix * translate(vec3(0, -10, 0));
            lineAssetRenderer.forward.pushModel(cmd, gridMatrix);
            grid.render(cmd);
        }



        /*if (texturedAssetRenderer.forward.bind(cmd))
        {
            texturedAssetRenderer.forward.pushModel(cmd, translate(vec3(-10.f, 1, -5.f)));
            texturedAssetRenderer.forward.bindTexture(cmd, box.descriptor);
            box.render(cmd);
        }*/
    }
    if (textureDisplay.forward.bind(cmd))
    {
        textureDisplay.forward.renderTexture(cmd, textureDes, vec2(10, 10), vec2(100, 50));
    }
}


void VulkanExample::renderGUI()
{
    ImGui::SetNextWindowSize(ImVec2(400, 250));
    ImGui::Begin("Example settings");
    ImGui::Checkbox("Render models", &displayModels);



    if (ImGui::Button("change point cloud"))
    {
        change = true;
    }


    if (ImGui::Button("reload shader"))
    {
        //        texturedAssetRenderer.shaderPipeline.reload();
        texturedAssetRenderer.reload();
        assetRenderer.reload();
        lineAssetRenderer.reload();
        textureDisplay.reload();
        pointCloudRenderer.reload();
        renderer->reload();
    }

    ImGui::Checkbox("Rotate Lights", &lightRotate);
    ImGui::DragFloat("Light Radius", &lightRadius, 0.25f, 0.f, 20.f);
    ImGui::DragFloat("Spot Opening Angle", &spotLightOpeningAngle, 0.25f, 0.f, 360.f);
    ImGui::DragFloat("Directional Light Intensity", &dirLightIntensity, 0.0125f, 0.f, 5.f);
    ImGui::DragFloat("Directional Ambient Intensity", &dirLightAmbientIntensity, 0.00125f, 0.f, 2.f);

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
