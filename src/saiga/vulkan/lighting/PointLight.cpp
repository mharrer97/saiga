/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */


/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/core/imgui/imgui.h"
#include "saiga/core/util/assert.h"
//#include "saiga/opengl/error.h"
//#include "saiga/opengl/rendering/deferredRendering/deferredRendering.h"
#include "saiga/vulkan/lighting/PointLight.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
/*void PointLightShader::checkUniforms()
{
    AttenuatedLightShader::checkUniforms();
    location_shadowPlanes = getUniformLocation("shadowPlanes");
}



void PointLightShader::uploadShadowPlanes(float f, float n)
{
    Shader::upload(location_shadowPlanes, vec2(f, n));
}*/


PointLight::PointLight() {}


PointLight& PointLight::operator=(const PointLight& light)
{
    model         = light.model;
    colorDiffuse  = light.colorDiffuse;
    colorSpecular = light.colorSpecular;
    attenuation   = light.attenuation;
    cutoffRadius  = light.cutoffRadius;
    return *this;
}


float PointLight::getRadius() const
{
    return cutoffRadius;
}


void PointLight::setRadius(float value)
{
    cutoffRadius = value;
    this->setScale(make_vec3(cutoffRadius));
}

/*void PointLight::bindUniforms(std::shared_ptr<PointLightShader> shader, Camera* cam)
{
    AttenuatedLight::bindUniforms(shader, cam);
    shader->uploadShadowPlanes(this->shadowCamera.zFar, this->shadowCamera.zNear);
    shader->uploadInvProj(inverse(cam->proj));
    if (this->hasShadows())
    {
        shader->uploadDepthBiasMV(viewToLightTransform(*cam, this->shadowCamera));
        shader->uploadDepthTexture(shadowmap->getDepthTexture());
        shader->uploadShadowMapSize(shadowmap->getSize());
    }
    assert_no_glerror();
}*/



/*void PointLight::createShadowMap(int w, int h, ShadowQuality quality)
{
    shadowmap = std::make_shared<CubeShadowmap>(w, h, quality);
    //    shadowmap->createCube(w,h);
}*/



/*struct CameraDirection
{
    GLenum CubemapFace;
    vec3 Target;
    vec3 Up;
};

static const CameraDirection gCameraDirections[] = {
    {GL_TEXTURE_CUBE_MAP_POSITIVE_X, vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_X, vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Y, vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Z, vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f)}};


void PointLight::bindFace(int face)
{
    shadowmap->bindCubeFace(gCameraDirections[face].CubemapFace);
}
*/

/*void PointLight::calculateCamera(int face)
{
    vec3 pos(this->getPosition());
    vec3 dir(gCameraDirections[face].Target);
    vec3 up(gCameraDirections[face].Up);
    shadowCamera.setView(pos, pos + dir, up);
    shadowCamera.setProj(90.0f, 1, shadowNearPlane, cutoffRadius);
}*/

bool PointLight::cullLight(Camera* cam)
{
    Sphere s(getPosition(), cutoffRadius);
    this->culled = cam->sphereInFrustum(s) == Camera::OUTSIDE;
    //    this->culled = false;
    //    std::cout<<culled<<endl;
    return culled;
}

/*
bool PointLight::renderShadowmap(DepthFunction f, UniformBuffer& shadowCameraBuffer)
{
    if (shouldCalculateShadowMap())
    {
        for (int i = 0; i < 6; i++)
        {
            bindFace(i);
            calculateCamera(i);
            shadowCamera.recalculatePlanes();
            CameraDataGLSL cd(&shadowCamera);
            shadowCameraBuffer.updateBuffer(&cd, sizeof(CameraDataGLSL), 0);
            f(&shadowCamera);
            shadowmap->unbindFramebuffer();
        }
        return true;
    }
    else
    {
        return false;
    }
}

void PointLight::renderImGui()
{
    AttenuatedLight::renderImGui();
    ImGui::InputFloat("shadowNearPlane", &shadowNearPlane);
}*/

}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga



namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
OldPointLight::OldPointLight(vec3 pos, vec3 dir, float angle) : light_pos(pos)
{
    this->light_dir = normalize(dir);
    this->angle     = clamp(angle, 0.f, 180.f);
}

//!
//! \brief PointLight::setPosition sets the position of the pointlight
//! \param pos
//!
void OldPointLight::setPosition(vec3 pos)
{
    light_pos = pos;
}

//!
//! \brief PointLight::setDirection sets the lights direction to the normalized incoming parameter
//! \param dir
//! \return normalized direction
//!
vec3 OldPointLight::setDirection(vec3 dir)
{
    light_dir = normalize(dir);
    return light_dir;
}

//!
//! \brief PointLight::setAngle sets lights opening angle to the given angle alamped to 0 to 180 degrees
//! \param angle
//! \return the clamped angle
//!
float OldPointLight::setAngle(float angle)
{
    this->angle = clamp(angle, 0.f, 180.f);
    return this->angle;
}

//!
//! \brief PointLight::getPosition
//! \return position of the light
//!
vec3 OldPointLight::getPosition()
{
    return light_pos;
}

//!
//! \brief PointLight::getDirection
//! \return direction of the light
//!
vec3 OldPointLight::getDirection()
{
    return light_dir;
}

//!
//! \brief PointLight::getAngle
//! \return opening angle of the light
//!
float OldPointLight::getAngle()
{
    return angle;
}
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
