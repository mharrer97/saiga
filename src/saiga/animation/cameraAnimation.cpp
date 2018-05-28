/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/animation/cameraAnimation.h"
#include "saiga/assets/assetLoader.h"
#include "saiga/imgui/imgui.h"

namespace Saiga {



Interpolation::Keyframe Interpolation::get(double time)
{

    int frame = Saiga::iCeil(time);

    int prevFrame = glm::max(0,frame - 1);



    float alpha = glm::fract(time);

    //    cout << "Interpolation " << prevFrame << "," << frame << " " << time << " " << alpha << endl;
    if(alpha == 0)
        return keyframes[frame];


    if(cubicInterpolation)
    {

        int if0 = glm::max(0,prevFrame-1);
        int if1 = prevFrame;
        int if2 = frame;
        int if3 = glm::min((int)keyframes.size()-1,frame+1);


        Keyframe& f0 = keyframes[if0];
        Keyframe& f1 = keyframes[if1];
        Keyframe& f2 = keyframes[if2];
        Keyframe& f3 = keyframes[if3];


        return interpolate(f0,f1,f2,f3,alpha);
    }

    else{
        Keyframe& f1 = keyframes[prevFrame];
        Keyframe& f2 = keyframes[frame];
        return interpolate(f1,f2,alpha);

    }
}



Interpolation::Keyframe Interpolation::getNormalized(double time)
{
    time = glm::clamp(time,0.0,1.0);
    return get( (keyframes.size()-1)*time);
}

std::shared_ptr<Asset> Interpolation::createAsset()
{
    if(keyframes.size() == 0)
        return nullptr;

    std::vector<vec3> vertices;
    std::vector<GLuint> indices;

    //    createFrustumMesh(proj,vertices,indices);

    int subSamples = 5;

    float scale = 0.5;

    for(int i = 0; i < keyframes.size()-1; ++i)
    {




        for(int j = (i==0)? -1 : 0; j < (subSamples + 1); ++j)
        {
            float alpha = (j+1.0) / (subSamples+1);

            float time = i + alpha;
            Keyframe kf = get(time);
            vec3 p = kf.position;

//            cout << "time " << time << " p " << p << endl;

            int idx = vertices.size();
            vertices.push_back(p);
            if(j != -1)
            {
                indices.push_back(idx - 4);
                indices.push_back(idx);
            }



            vertices.push_back( p + scale * (kf.rot * vec3(1,0,0)) );
            vertices.push_back( p + scale * (kf.rot * vec3(0,1,0)) );
            vertices.push_back( p + scale * (kf.rot * vec3(0,0,1)) );

            indices.push_back(idx); indices.push_back(idx+1);
            indices.push_back(idx); indices.push_back(idx+2);
            indices.push_back(idx); indices.push_back(idx+3);
        }

    }




    AssetLoader al;
    return al.nonTriangleMesh(vertices,indices,GL_LINES,vec4(1,0,0,1));
}


Saiga::Interpolation::Keyframe Saiga::Interpolation::interpolate(const Saiga::Interpolation::Keyframe &f1, const Saiga::Interpolation::Keyframe &f2, const Saiga::Interpolation::Keyframe &f3, const Saiga::Interpolation::Keyframe &f4, float alpha)
{

    float tau = 1;

    Keyframe res;


    float u = alpha;
    float u2 = u * u;
    float u3 = u2 * u;

    mat4 A = {
        0,2,0,0,
        -1,0,1,0,
        2,-5,4,-1,
        -1,3,-3,1
    };
    A = mat4(transpose(A));


    //        cout << A << endl;
    vec3 ps[4] = {f1.position,f2.position,f3.position,f4.position};

    vec3 ps2[4];


    for(int i = 0; i < 4; ++i)
    {
        vec3 p(0);
        for(int j = 0; j < 4; ++j)
        {
            p += A[j][i] * ps[j];
        }
        ps2[i] = p;
        //            cout << "p " << p << endl;
    }


    res.position = 0.5f * (1.f * ps2[0] + u * ps2[1] + u2 * ps2[2] + u3 * ps2[3]);
    //        res.position =  glm::mix(f1.position,f2.position,alpha);


    res.rot =  glm::slerp(f2.rot,f3.rot,alpha);
    return res;
}

Saiga::Interpolation::Keyframe Saiga::Interpolation::interpolate(const Saiga::Interpolation::Keyframe &f1, const Saiga::Interpolation::Keyframe &f2, float alpha)
{
    Keyframe res;
    res.position =  glm::mix(f1.position,f2.position,alpha);
    res.rot =  glm::slerp(f1.rot,f2.rot,alpha);
    return res;
}

void Interpolation::start(Camera &cam, float totalTimeS, float dt)
{
    totalTicks = totalTimeS / dt;
    tick = 0;

    cout << "Starting Camera Interpolation. " << totalTimeS << "s  dt=" << dt << " TotalTicks: " << totalTicks << endl;

    update(cam);

}

bool Interpolation::update(Camera &camera)
{

    if(tick > totalTicks)
        return false;

    float cameraAlpha = float(tick) / totalTicks;
    auto kf = getNormalized(cameraAlpha);

    camera.position = vec4(kf.position,1);
    camera.rot = kf.rot;

    camera.calculateModel();
    camera.updateFromModel();

    cameraAlpha += 0.002;


    tick++;

    return true;
}

void Interpolation::render()
{

    if(cameraPathAsset && !isRunning())
    {
        cameraPathAsset->renderForward(nullptr,mat4(1));
    }
}
void Interpolation::renderGui(Camera& camera)
{
    bool changed = false;

    {
        ImGui::SetNextWindowPos(ImVec2(50, 400), ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400,200), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Camera");


        ImGui::InputFloat("dt",&dt);
        ImGui::InputFloat("totalTime",&totalTime);
        if(ImGui::Checkbox("cubicInterpolation",&cubicInterpolation))
        {
            changed = true;
        }


        if(ImGui::Button("Add Keyframe"))
        {
            addKeyframe(camera.rot,camera.getPosition());

            cameraPathAsset = createAsset();
            changed = true;
        }

        if(ImGui::Button("Remove Last Keyframe"))
        {
            keyframes.pop_back();
            changed = true;
        }

        if(ImGui::Button("Clear Keyframes"))
        {
            keyframes.clear();
            changed = true;
        }

        if(ImGui::Button("start camera"))
        {
            start(camera,totalTime,dt);
            changed = true;
        }

        if(changed)
        {
            cameraPathAsset = createAsset();
        }


        ImGui::End();
    }
}

}
