#include "rendering/lighting/deferred_lighting.h"
#include "util/inputcontroller.h"

#include "rendering/deferred_renderer.h"
#include "libhello/util/error.h"

#include "libhello/rendering/lighting/directional_light.h"
#include "libhello/rendering/lighting/point_light.h"
#include "libhello/rendering/lighting/spot_light.h"

#include "libhello/geometry/triangle_mesh_generator.h"
#include "libhello/opengl/texture/cube_texture.h"

DeferredLighting::DeferredLighting(Framebuffer &framebuffer):framebuffer(framebuffer){
    
    createInputCommands();
    createLightMeshes();



    dummyTexture = new Texture();
    dummyTexture->createEmptyTexture(1,1,GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16,GL_UNSIGNED_SHORT);
    dummyTexture->setWrap(GL_CLAMP_TO_EDGE);
    dummyTexture->setFiltering(GL_LINEAR);
     //this requires the texture sampler in the shader to be sampler2DShadow
    dummyTexture->setParameter(GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);
    dummyTexture->setParameter(GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);



    dummyCubeTexture = new cube_Texture();
    dummyCubeTexture->createEmptyTexture(1,1,GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16,GL_UNSIGNED_SHORT);
    dummyCubeTexture->setWrap(GL_CLAMP_TO_EDGE);
    dummyCubeTexture->setFiltering(GL_LINEAR);
     //this requires the texture sampler in the shader to be sampler2DShadow
    dummyCubeTexture->setParameter(GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);
    dummyCubeTexture->setParameter(GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);
}

DeferredLighting::~DeferredLighting(){
    delete dummyTexture;
    delete dummyCubeTexture;
    //delete all lights
    //the shader loader will delete the shaders.
//    for(PointLight* &obj : pointLights){
//        delete obj;
//    }
//    for(SpotLight* &obj : spotLights){
//        delete obj;
//    }
//    for(DirectionalLight* &obj : directionalLights){
//        delete obj;
//    }
}

void DeferredLighting::cullLights(Camera *cam){

    visibleLights = directionalLights.size();

    //cull lights that are not visible
    for(SpotLight* &light : spotLights){
        if(light->isActive()){
            light->calculateCamera();
            light->cam.recalculatePlanes();
            visibleLights += (light->cullLight(cam))? 0 : 1;
        }
    }


    for(PointLight* &light : pointLights){
        if(light->isActive()){
            visibleLights += (light->cullLight(cam))? 0 : 1;
        }
    }
}

void DeferredLighting::renderDepthMaps(RendererInterface *renderer){
    totalLights = 0;
    renderedDepthmaps = 0;

    totalLights = directionalLights.size() + spotLights.size() + pointLights.size();

    for(DirectionalLight* &light : directionalLights){

        if(light->shouldCalculateShadowMap()){
            renderedDepthmaps++;
            light->bindShadowMap();
            light->cam.recalculatePlanes();
            renderer->renderDepth(&light->cam);
            light->unbindShadowMap();
        }

    }

    for(SpotLight* &light : spotLights){
        if(light->shouldCalculateShadowMap()){
            renderedDepthmaps++;
            light->bindShadowMap();
            renderer->renderDepth(&light->cam);
            light->unbindShadowMap();
        }
    }


    for(PointLight* &light : pointLights){

        if(light->shouldCalculateShadowMap()){
            renderedDepthmaps+=6;
            for(int i=0;i<6;i++){
                light->bindFace(i);
                light->calculateCamera(i);
                light->cam.recalculatePlanes();
                renderer->renderDepth(&light->cam);
                light->unbindShadowMap();
            }

        }
    }

}

void DeferredLighting::render(Camera* cam){
    //viewport is maybe different after shadow map rendering
    glViewport(0,0,width,height);

    //deferred lighting uses additive blending of the lights.
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    //never overwrite current depthbuffer
    glDepthMask(GL_FALSE);

    //point- and spot- lights are using stencil culling
    glEnable(GL_STENCIL_TEST);


    renderSpotLightsStencil(); //mark pixels inside the light volume
    renderSpotLights(cam); //draw back faces without depthtest

    Error::quitWhenError("DeferredLighting::spotLights");

    renderPointLightsStencil(); //mark pixels inside the light volume
    renderPointLights(cam); //draw back faces without depthtest

    glDisable(GL_STENCIL_TEST);

    Error::quitWhenError("DeferredLighting::pointLights");

    //use default culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    renderDirectionalLights(cam);

    //reset state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    if(drawDebug){
        glDepthMask(GL_TRUE);
        renderDebug();
        glDepthMask(GL_FALSE);
    }



    Error::quitWhenError("DeferredLighting::lighting");

}

void DeferredLighting::setupStencilPass(){
    glEnable(GL_DEPTH_TEST);

    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);

    glClear(GL_STENCIL_BUFFER_BIT);

    glDisable(GL_CULL_FACE);

    // We need the stencil test to be enabled but we want it
    // to succeed always. Only the depth test matters.
    glStencilFunc(GL_ALWAYS, 0, 0);

    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP);


}
void DeferredLighting::setupLightPass(){
    // Disable color/depth write and enable stencil

    glStencilFunc(GL_NOTEQUAL, 0, 0xFF); //pass when pixel is inside a light volume
    glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP);//do nothing
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}


void DeferredLighting::renderPointLights(Camera *cam){

    setupLightPass();
    pointLightShader->bind();
    pointLightShader->uploadView(view);
    pointLightShader->uploadProj(proj);
    pointLightShader->DeferredShader::uploadFramebuffer(&framebuffer);
    pointLightShader->uploadScreenSize(vec2(width,height));

//    Error::quitWhenError("DeferredLighting::renderPointLights1");

    pointLightMesh.bind();
    for(PointLight* &obj : pointLights){
        if(obj->shouldRender()){
            obj->bindUniforms(*pointLightShader,cam);
            pointLightMesh.draw();
        }
    }
    pointLightMesh.unbind();
    pointLightShader->unbind();

      Error::quitWhenError("DeferredLighting::renderPointLights4");
}

void DeferredLighting::renderPointLightsStencil(){

    setupStencilPass();
    stencilShader->bind();
    stencilShader->uploadView(view);
    stencilShader->uploadProj(proj);
    pointLightMesh.bind();
    for(PointLight* &obj : pointLights){
        if(obj->shouldRender()){

            obj->bindUniformsStencil(*stencilShader);
            pointLightMesh.draw();
        }
    }
    pointLightMesh.unbind();
    stencilShader->unbind();
}


void DeferredLighting::renderSpotLights(Camera *cam){


    setupLightPass();

    spotLightShader->bind();
    spotLightShader->uploadView(view);
    spotLightShader->uploadProj(proj);
    spotLightShader->DeferredShader::uploadFramebuffer(&framebuffer);
    spotLightShader->uploadScreenSize(vec2(width,height));

    spotLightMesh.bind();
    for(SpotLight* &obj : spotLights){
        if(obj->shouldRender()){
            obj->bindUniforms(*spotLightShader,cam);
            spotLightMesh.draw();
        }
    }
    spotLightMesh.unbind();
    spotLightShader->unbind();
}

void DeferredLighting::renderSpotLightsStencil(){
    setupStencilPass();

    stencilShader->bind();
    stencilShader->uploadView(view);
    stencilShader->uploadProj(proj);
    spotLightMesh.bind();
    for(SpotLight* &obj : spotLights){
        if(obj->shouldRender()){

            obj->bindUniformsStencil(*stencilShader);
            spotLightMesh.draw();
        }
    }
    spotLightMesh.unbind();
    stencilShader->unbind();
}


void DeferredLighting::renderDirectionalLights(Camera *cam){


    directionalLightShader->bind();
    directionalLightShader->uploadView(view);
    directionalLightShader->uploadProj(proj);
    directionalLightShader->DeferredShader::uploadFramebuffer(&framebuffer);
    directionalLightShader->uploadScreenSize(vec2(width,height));
    directionalLightShader->uploadSsaoTexture(ssaoTexture);

    directionalLightMesh.bind();
    for(DirectionalLight* &obj : directionalLights){
        if(obj->shouldRender()){
            obj->view = &view;
            obj->bindUniforms(*directionalLightShader,cam);
            directionalLightMesh.draw();
        }
    }
    directionalLightMesh.unbind();
    directionalLightShader->unbind();
}


void DeferredLighting::renderDebug(){

    debugShader->bind();
    debugShader->uploadView(view);
    debugShader->uploadProj(proj);
    //    debugShader->uploadFramebuffer(&framebuffer);
    //    debugShader->uploadScreenSize(vec2(width,height));

    pointLightMesh.bind();
    //center
    for(PointLight* &obj : pointLights){
        mat4 sm = glm::scale(obj->model,vec3(0.05));
        vec4 color = obj->color;
        if(!obj->isActive()||!obj->isVisible()){
            //render as black if light is turned off
            color = vec4(0);
        }
        debugShader->uploadModel(sm);
        debugShader->uploadColor(color);
        pointLightMesh.draw();
    }

    //render outline
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    for(PointLight* &obj : pointLights){
        if(obj->isSelected()){
            debugShader->uploadModel(obj->model);
            debugShader->uploadColor(obj->color);
            pointLightMesh.draw();
        }
    }
    pointLightMesh.unbind();
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );


    spotLightMesh.bind();
    //center
    for(SpotLight* &obj : spotLights){
        mat4 sm = glm::scale(obj->model,vec3(0.05));
        vec4 color = obj->color;
        if(!obj->isActive()||!obj->isVisible()){
            //render as black if light is turned off
            color = vec4(0);
        }
        debugShader->uploadModel(sm);
        debugShader->uploadColor(color);
        spotLightMesh.draw();
    }

    //render outline
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    for(SpotLight* &obj : spotLights){
        if(obj->isSelected()){
            debugShader->uploadModel(obj->model);
            debugShader->uploadColor(obj->color);
            spotLightMesh.draw();
        }
    }
    spotLightMesh.unbind();
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    debugShader->unbind();

}


void DeferredLighting::setShader(SpotLightShader* spotLightShader){
    this->spotLightShader = spotLightShader;
}

void DeferredLighting::setShader(PointLightShader* pointLightShader){
    this->pointLightShader = pointLightShader;
}

void DeferredLighting::setShader(DirectionalLightShader* directionalLightShader){
    this->directionalLightShader = directionalLightShader;
}

void DeferredLighting::setDebugShader(MVPColorShader *shader){
    this->debugShader = shader;
}

void DeferredLighting::setStencilShader(MVPShader* stencilShader){
    this->stencilShader = stencilShader;
}

void DeferredLighting::createInputCommands(){

}

void DeferredLighting::createLightMeshes(){

    auto qb = TriangleMeshGenerator::createFullScreenQuadMesh();
    qb->createBuffers(directionalLightMesh);

    Sphere s(vec3(0),1);
    auto sb = TriangleMeshGenerator::createMesh(s,1);
    sb->createBuffers(pointLightMesh);



    Cone c(vec3(0),vec3(0,1,0),30.0f,1.0f);
    auto cb = TriangleMeshGenerator::createMesh(c,10);
    cb->createBuffers(spotLightMesh);
}

DirectionalLight* DeferredLighting::createDirectionalLight(){
    DirectionalLight* l = new DirectionalLight();
    l->dummyTexture = dummyTexture;
    directionalLights.push_back(l);
    return l;
}

PointLight* DeferredLighting::createPointLight(){
    PointLight* l = new PointLight();
    l->dummyTexture = dummyCubeTexture;
    pointLights.push_back(l);
    return l;
}

SpotLight* DeferredLighting::createSpotLight(){
    SpotLight* l = new SpotLight();
    l->dummyTexture = dummyTexture;
    spotLights.push_back(l);
    return l;
}

void DeferredLighting::removeDirectionalLight(DirectionalLight *l)
{
    directionalLights.erase(std::find(directionalLights.begin(),directionalLights.end(),l));
}

void DeferredLighting::removePointLight(PointLight *l)
{
    pointLights.erase(std::find(pointLights.begin(),pointLights.end(),l));
}

void DeferredLighting::removeSpotLight(SpotLight *l)
{
    spotLights.erase(std::find(spotLights.begin(),spotLights.end(),l));
}

void DeferredLighting::setViewProj(const mat4 &iv,const mat4 &v,const mat4 &p)
{
    inview = iv;
    view = v;
    proj = p;
}
