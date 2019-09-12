/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 *
 */

#include "saiga/vulkan/lighting/AttenuatedLight.h"

#include "saiga/core/imgui/imgui.h"
#include "saiga/core/util/assert.h"
//#include "saiga/opengl/error.h"
namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
/*void AttenuatedLightShader::checkUniforms()
{
    LightShader::checkUniforms();
    location_attenuation = getUniformLocation("attenuation");
}


void AttenuatedLightShader::uploadA(vec3& attenuation, float cutoffRadius)
{
    Shader::upload(location_attenuation, make_vec4(attenuation, cutoffRadius));
}*/

AttenuatedLight::AttenuatedLight() {}

AttenuatedLight& AttenuatedLight::operator=(const AttenuatedLight& light)
{
    model         = light.model;
    colorDiffuse  = light.colorDiffuse;
    colorSpecular = light.colorSpecular;
    attenuation   = light.attenuation;
    cutoffRadius  = light.cutoffRadius;
    return *this;
}

float AttenuatedLight::evaluateAttenuation(float distance)
{
    // normalize the distance, so the attenuation is independent of the radius
    float x = distance / cutoffRadius;

    return 1.0f / (attenuation[0] + attenuation[1] * x + attenuation[2] * x * x);
}

vec3 AttenuatedLight::getAttenuation() const
{
    return attenuation;
}

float AttenuatedLight::getAttenuation(float r)
{
    float x = r / cutoffRadius;
    return 1.0 / (attenuation[0] + attenuation[1] * x + attenuation[2] * x * x);
}

void AttenuatedLight::setAttenuation(const vec3& value)
{
    attenuation = value;
}


float AttenuatedLight::getRadius() const
{
    return cutoffRadius;
}


void AttenuatedLight::setRadius(float value)
{
    cutoffRadius = value;
    this->setScale(make_vec3(cutoffRadius));
}

/*void AttenuatedLight::bindUniforms(std::shared_ptr<AttenuatedLightShader> shader, Camera* cam)
{
    if (isVolumetric()) shader->uploadVolumetricDensity(volumetricDensity);
    shader->uploadColorDiffuse(colorDiffuse);
    shader->uploadColorSpecular(colorSpecular);
    shader->uploadModel(model);
    shader->uploadA(attenuation, cutoffRadius);
    assert_no_glerror();
}


void AttenuatedLight::renderImGui()
{
    Light::renderImGui();
    ImGui::InputFloat3("attenuation", &attenuation[0]);
    float c = evaluateAttenuation(cutoffRadius);
    ImGui::Text("Cutoff Intensity: %f", c);
    ImGui::InputFloat("cutoffRadius", &cutoffRadius);
}
*/

}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
