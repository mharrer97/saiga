/**
 * Copyright (c) 2017 Darius Rückert 
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/rendering/lighting/light.h"
#include "saiga/camera/camera.h"
#include "saiga/imgui/imgui.h"

namespace Saiga {

void LightShader::checkUniforms(){
    DeferredShader::checkUniforms();
    location_lightColorDiffuse = getUniformLocation("lightColorDiffuse");
    location_lightColorSpecular = getUniformLocation("lightColorSpecular");
    location_depthBiasMV = getUniformLocation("depthBiasMV");
    location_depthTex = getUniformLocation("depthTex");
    location_readShadowMap = getUniformLocation("readShadowMap");
    location_shadowMapSize = getUniformLocation("shadowMapSize");
    location_invProj = getUniformLocation("invProj");
}


void LightShader::uploadColorDiffuse(vec4 &color){
    Shader::upload(location_lightColorDiffuse,color);
}

void LightShader::uploadColorDiffuse(vec3 &color, float intensity){
    vec4 c = vec4(color,intensity);
    Shader::upload(location_lightColorDiffuse,c);
}

void LightShader::uploadColorSpecular(vec4 &color){
    Shader::upload(location_lightColorSpecular,color);
}

void LightShader::uploadColorSpecular(vec3 &color, float intensity){
    vec4 c = vec4(color,intensity);
    Shader::upload(location_lightColorSpecular,c);
}

void LightShader::uploadDepthBiasMV(const mat4 &mat){
    Shader::upload(location_depthBiasMV,mat);
}

void LightShader::uploadInvProj(const mat4 &mat){
    Shader::upload(location_invProj,mat);
}

void LightShader::uploadDepthTexture(std::shared_ptr<raw_Texture> texture){
    texture->bind(5);
    Shader::upload(location_depthTex,5);
}

void LightShader::uploadShadow(float shadow){
    Shader::upload(location_readShadowMap,shadow);
}

void LightShader::uploadShadowMapSize(glm::ivec2 s)
{
    auto w = s.x;
    auto h = s.y;
    Shader::upload(location_shadowMapSize,vec4(w,h,1.0f/w,1.0f/h));
}

//void Light::createShadowMap(int resX, int resY){

////    cout<<"Light::createShadowMap"<<endl;

//    shadowmap.createFlat(resX,resY);

//}


void Light::bindUniformsStencil(MVPShader& shader){
    shader.uploadModel(model);
}

mat4 Light::viewToLightTransform(const Camera &camera, const Camera &shadowCamera)
{
    //glm like glsl is column major!
    const mat4 biasMatrix(
                0.5, 0.0, 0.0, 0.0,
                0.0, 0.5, 0.0, 0.0,
                0.0, 0.0, 0.5, 0.0,
                0.5, 0.5, 0.5, 1.0
                );
    //We could also use inverse(camera.view) but using the model matrix is faster
    return biasMatrix * shadowCamera.proj * shadowCamera.view * camera.model;
}

void Light::renderImGui()
{
    ImGui::Checkbox("active",&active);
    ImGui::Checkbox("castShadows",&castShadows);
    ImGui::InputFloat("intensity",&colorDiffuse.w,0.1,1);
    ImGui::ColorEdit3("colorDiffuse",&colorDiffuse[0]);
    ImGui::ColorEdit3("colorSpecular",&colorSpecular[0]);
}

}
