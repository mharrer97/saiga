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

VulkanExample::VulkanExample(Saiga::Vulkan::VulkanWindow& window, Saiga::Vulkan::VulkanDeferredRenderer& renderer)
    : Updating(window), Saiga::Vulkan::VulkanDeferredRenderingInterface(renderer), renderer(renderer)
{
    float aspect = window.getAspectRatio();
    camera.setProj(60.0f, aspect, 0.1f, 50.0f, true);
    camera.setView(vec3(0, 5, 10), vec3(0, 0, 0), vec3(0, 1, 0));
    camera.rotationPoint = make_vec3(0);


    //   std::cout << camera.view <<std::endl;
    //   std::cout << camera.proj <<std::endl;
    //    exit(0);

    window.setCamera(&camera);

    init(renderer.base());
}

VulkanExample::~VulkanExample()
{
    LOG(INFO) << "~VulkanExample";
    assetRenderer.destroy();
    lineAssetRenderer.destroy();
    pointCloudRenderer.destroy();
    texturedAssetRenderer.destroy();
}

void VulkanExample::init(Saiga::Vulkan::VulkanBase& base)
{
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


    assetRenderer.deferred.init(base, renderer.renderPass);
    assetRenderer.forward.init(base, renderer.forwardPass);
    lineAssetRenderer.deferred.init(base, renderer.renderPass, 2);
    lineAssetRenderer.forward.init(base, renderer.forwardPass, 2);
    pointCloudRenderer.deferred.init(base, renderer.renderPass, 5);
    pointCloudRenderer.forward.init(base, renderer.forwardPass, 5);
    texturedAssetRenderer.deferred.init(base, renderer.renderPass);
    texturedAssetRenderer.forward.init(base, renderer.forwardPass);
    textureDisplay.deferred.init(base, renderer.renderPass);
    textureDisplay.forward.init(base, renderer.forwardPass);

    textureDes = textureDisplay.forward.createAndUpdateDescriptorSet(*texture);


    box.loadObj("box.obj");

    ////    box.loadObj("cat.obj");
    box.init(renderer.base());
    box.descriptor = texturedAssetRenderer.forward.createAndUpdateDescriptorSet(*box.textures[0]);


    teapot.loadObj("teapot.obj");
    //        teapot.loadPly("dragon_10k.ply");
    //    teapot.loadPly("fr3_office.ply");
    teapot.mesh.computePerVertexNormal();
    teapot.init(renderer.base());
    teapotTrans.setScale(vec3(2, 2, 2));
    //    teapotTrans.rotateGlobal(vec3(1, 0, 0), pi<float>());
    teapotTrans.translateGlobal(vec3(0, 2, 0));
    teapotTrans.calculateModel();

    plane.createCheckerBoard(ivec2(20, 20), 1.0f, Saiga::Colors::firebrick, Saiga::Colors::gray);
    plane.init(renderer.base());

    sphere.loadObj("icosphere.obj");
    sphere.init(renderer.base());

    grid.createGrid(10, 10);
    grid.init(renderer.base());

    frustum.createFrustum(camera.proj, 2, make_vec4(1), true);
    frustum.init(renderer.base());

    pointCloud.init(base, 1000 * 1000);
    for (int i = 0; i < 1000 * 1000; ++i)
    {
        Saiga::VertexNC v;
        v.position               = make_vec4(linearRand(make_vec3(-3), make_vec3(3)), 1);
        v.color                  = make_vec4(linearRand(make_vec3(0), make_vec3(1)), 1);
        pointCloud.pointCloud[i] = v;
    }


    std::shared_ptr<Saiga::Vulkan::Lighting::PointLight> pointTestLight;
    int count = 0;
    for (int i = 0; i < count; ++i)
    {
        vec3 pos = vec3(sin((float(i) / float(count)) * 6.28f), 1.f, cos((float(i) / float(count)) * 6.28f));
        pos *= 15.f;

        // pos                           = vec3(1.f, 0.f, 1.f) * (3 * float(i));
        pos[1]                   = 5.f;
        pointTestLight           = renderer.lighting.createPointLight();
        pointTestLight->position = pos;
        pointLights.push_back(pointTestLight);
    }

    spotLight = renderer.lighting.createSpotLight();
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


    if (renderer.lightRotate)
    {
        timingLoop = fmod(timingLoop + dt, 2.f * 3.1415f);
        vec3 pos   = vec3(sin(timingLoop) * 7.5f, 7.5f, cos(timingLoop) * 7.5f);
        renderer.pointLight.setPosition(pos);
        renderer.pointLight.setDirection(-pos);

        int count = pointLights.size();
        for (int i = 0; i < count; ++i)
        {
            vec3 pos = vec3(sin(fmod(((float(i) / float(count)) * 6.28f) - timingLoop, 2.f * 3.1415f)), 1.f,
                            cos(fmod(((float(i) / float(count)) * 6.28f) - timingLoop, 2.f * 3.1415f)));
            pos *= 15.f;

            // pos                           = vec3(1.f, 0.f, 1.f) * (3 * float(i));
            pos[1]                   = 5.f;
            pointLights[i]->position = pos;
        }
    }

    int count = pointLights.size();
    for (int i = 0; i < count; ++i)
    {
        pointLights[i]->setRadius(lightRadius);
    }

    vec3 pos =
        vec3(sin(fmod(3.1415f - timingLoop, 2.f * 3.1415f)), 1.f, cos(fmod(3.1415f - timingLoop, 2.f * 3.1415f)));
    pos *= 10.f;
    spotLight->setDirection(-pos);
    spotLight->setRadius(lightRadius);
    pos[1]              = 3.5f;
    spotLight->position = pos;
    spotLight->setOpeningAngle(spotLightOpeningAngle);
}

void VulkanExample::transfer(vk::CommandBuffer cmd, Camera* cam)
{
    assetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    pointCloudRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    lineAssetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
    texturedAssetRenderer.deferred.updateUniformBuffers(cmd, cam->view, cam->proj);
}

void VulkanExample::transferForward(vk::CommandBuffer cmd, Camera* cam)
{
    assetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    pointCloudRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    lineAssetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);
    texturedAssetRenderer.forward.updateUniformBuffers(cmd, cam->view, cam->proj);

    // upload everything every frame
    if (uploadChanges)
    {
        pointCloud.updateBuffer(cmd, 0, pointCloud.capacity);

        uploadChanges = false;
    }
}

void VulkanExample::render(vk::CommandBuffer cmd, Camera* cam)
{
    if (displayModels)
    {
        if (assetRenderer.deferred.bind(cmd))
        {
            assetRenderer.deferred.pushModel(cmd, identityMat4());
            plane.render(cmd);

            assetRenderer.deferred.pushModel(cmd, teapotTrans.model);
            teapot.render(cmd);
        }

        if (pointCloudRenderer.deferred.bind(cmd))
        {
            pointCloudRenderer.deferred.pushModel(cmd, translate(vec3(10, 2.5f, 0)));
            pointCloud.render(cmd, 0, pointCloud.capacity);
        }

        if (lineAssetRenderer.deferred.bind(cmd))
        {
            lineAssetRenderer.deferred.pushModel(cmd, translate(vec3(-10.f, 1.5f, 0)));
            teapot.render(cmd);

            auto gridMatrix = rotate(0.5f * pi<float>(), vec3(1, 0, 0));
            gridMatrix      = translate(gridMatrix, vec3(0, -10, 0));
            lineAssetRenderer.deferred.pushModel(cmd, gridMatrix);
            grid.render(cmd);
        }

        if (texturedAssetRenderer.deferred.bind(cmd))
        {
            texturedAssetRenderer.deferred.pushModel(cmd, translate(vec3(-10.f, 1, -5.f)));
            texturedAssetRenderer.deferred.bindTexture(cmd, box.descriptor);
            box.render(cmd);
        }

        if (textureDisplay.deferred.bind(cmd))
        {
            textureDisplay.deferred.renderTexture(cmd, textureDes, vec2(150, 10), vec2(100, 50));
        }
    }
}

void VulkanExample::renderForward(vk::CommandBuffer cmd, Camera* cam)
{
    if (displayModels)
    {
        if (assetRenderer.forward.bind(cmd))
        {
            assetRenderer.forward.pushModel(cmd, identityMat4());
            // plane.render(cmd);



            assetRenderer.forward.pushModel(cmd, teapotTrans.model * translate(vec3(5.f, 0.f, 5.f)));
            teapot.render(cmd);


            assetRenderer.forward.pushModel(
                cmd, scale(translate(renderer.pointLight.getPosition()), vec3(0.1f, 0.1f, 0.1f)));
            sphere.render(cmd);
            for (auto& l : pointLights)
            {
                assetRenderer.forward.pushModel(cmd, scale(translate(l->position), vec3(0.1f, 0.1f, 0.1f)));
                sphere.render(cmd);
            }
            assetRenderer.forward.pushModel(cmd, scale(translate(spotLight->position), vec3(0.1f, 0.1f, 0.1f)));
            sphere.render(cmd);
        }
        if (pointCloudRenderer.forward.bind(cmd))
        {
            pointCloudRenderer.forward.pushModel(cmd, translate(vec3(10, 2.5f, 0)));
            // pointCloud.render(cmd, 0, pointCloud.capacity);
        }
        if (lineAssetRenderer.forward.bind(cmd))
        {
            lineAssetRenderer.forward.pushModel(cmd, translate(vec3(-10.f, 1.5f, 0)));
            // teapot.render(cmd);

            auto gridMatrix = rotate(0.5f * pi<float>(), vec3(1, 0, 0));
            gridMatrix      = translate(gridMatrix, vec3(0, -10, 0));
            lineAssetRenderer.forward.pushModel(cmd, gridMatrix);
            // grid.render(cmd);
        }



        if (texturedAssetRenderer.forward.bind(cmd))
        {
            texturedAssetRenderer.forward.pushModel(cmd, translate(vec3(-7.5f, 1, -5.f)));
            texturedAssetRenderer.forward.bindTexture(cmd, box.descriptor);
            box.render(cmd);
        }
    }
    if (textureDisplay.forward.bind(cmd))
    {
        textureDisplay.forward.renderTexture(cmd, textureDes, vec2(10, 10), vec2(100, 50));
    }
}


void VulkanExample::renderGUI()
{
    ImGui::SetNextWindowSize(ImVec2(200, 200));
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
        renderer.reload();
    }

    ImGui::DragFloat("Light Radius", &lightRadius, 0.25f, 0.f, 20.f);
    ImGui::DragFloat("Spot Opening Angle", &spotLightOpeningAngle, 0.25f, 0.f, 360.f);
    ImGui::End();
    //    return;

    parentWindow.renderImGui();
    //    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
    //    ImGui::ShowTestWindow();
}


void VulkanExample::keyPressed(SDL_Keysym key)
{
    switch (key.scancode)
    {
        case SDL_SCANCODE_ESCAPE:
            parentWindow.close();
            break;
        default:
            break;
    }
}

void VulkanExample::keyReleased(SDL_Keysym key) {}
