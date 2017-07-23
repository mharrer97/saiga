/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/rendering/lighting/box_light.h"

namespace Saiga {

void BoxLightShader::checkUniforms(){
    LightShader::checkUniforms();
}


//==================================


BoxLight::BoxLight()
{
}

void BoxLight::createShadowMap(int resX, int resY){
    shadowmap.createFlat(resX,resY);
}


void BoxLight::bindUniforms(std::shared_ptr<BoxLightShader> shader, Camera *cam){
    shader->uploadColorDiffuse(colorDiffuse);
    shader->uploadColorSpecular(colorSpecular);
    shader->uploadModel(model);
    shader->uploadInvProj(glm::inverse(cam->proj));
    if(this->hasShadows()){
        shader->uploadDepthBiasMV(viewToLightTransform(*cam,this->shadowCamera));
        shader->uploadDepthTexture(shadowmap.getDepthTexture(0));
        shader->uploadShadowMapSize(shadowmap.getSize());
    }
}

void BoxLight::setView(vec3 pos, vec3 target, vec3 up)
{
    this->setViewMatrix(glm::lookAt(pos,pos +  (pos-target),up));
}

void BoxLight::calculateCamera(){
    vec3 dir = glm::normalize(vec3(this->getDirection()));
    vec3 pos = getPosition() ;
    vec3 up = vec3(getUpVector());

    //the camera is centred at the centre of the shadow volume.
    //we define the box only by the sides of the orthographic projection
    shadowCamera.setView(pos,pos+dir,up);
    shadowCamera.setProj(-scale.x,scale.x,-scale.y,scale.y,-scale.z,scale.z);
}

bool BoxLight::cullLight(Camera *cam)
{
    //do an exact frustum-frustum intersection if this light casts shadows, else do only a quick check.
    if(this->hasShadows())
        this->culled = !this->shadowCamera.intersectSAT(cam);
    else
        this->culled = cam->sphereInFrustum(this->shadowCamera.boundingSphere)==Camera::OUTSIDE;
    return culled;
}

}
