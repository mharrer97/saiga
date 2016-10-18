#include "saiga/smaa/SMAA.h"
#include "saiga/smaa/AreaTex.h"
#include "saiga/smaa/SearchTex.h"

#include "saiga/opengl/shader/shaderLoader.h"
#include "saiga/geometry/triangle_mesh_generator.h"
#include "saiga/opengl/texture/imageGenerator.h"
#include "saiga/rendering/gbuffer.h"

void SMAABlendingWeightCalculationShader::checkUniforms()
{
    Shader::checkUniforms();
    location_edgeTex = Shader::getUniformLocation("edgeTex");
    location_areaTex = Shader::getUniformLocation("areaTex");
    location_searchTex = Shader::getUniformLocation("searchTex");

}

void SMAABlendingWeightCalculationShader::uploadTextures(raw_Texture *edgeTex, raw_Texture *areaTex, raw_Texture *searchTex)
{
    edgeTex->bind(0);
    Shader::upload(location_edgeTex,0);

    areaTex->bind(1);
    Shader::upload(location_areaTex,1);

    searchTex->bind(2);
    Shader::upload(location_searchTex,2);
}

void SMAANeighborhoodBlendingShader::checkUniforms()
{
    Shader::checkUniforms();
    location_colorTex = Shader::getUniformLocation("colorTex");
    location_blendTex = Shader::getUniformLocation("blendTex");
}

void SMAANeighborhoodBlendingShader::uploadTextures(raw_Texture *colorTex, raw_Texture *blendTex)
{
    colorTex->bind(0);
    Shader::upload(location_colorTex,0);

    blendTex->bind(1);
    Shader::upload(location_blendTex,1);
}


SMAA::SMAA(int w, int h, Quality _quality) : screenSize(w,h) , quality(_quality)
{

    stencilTex = framebuffer_texture_t(new Texture());
    stencilTex->createEmptyTexture(w,h,GL_STENCIL_INDEX,GL_STENCIL_INDEX8,GL_UNSIGNED_BYTE);

    edgesTex = framebuffer_texture_t(new Texture());
    edgesTex->createEmptyTexture(w,h,GL_RGBA,GL_RGBA8,GL_UNSIGNED_BYTE);
    edgesFb.create();
    edgesFb.attachTexture( edgesTex);
    edgesFb.attachTextureStencil(stencilTex);
    edgesFb.drawToAll();
    edgesFb.check();
    edgesFb.unbind();

    blendTex = framebuffer_texture_t(new Texture());
    blendTex->createEmptyTexture(w,h,GL_RGBA,GL_RGBA8,GL_UNSIGNED_BYTE);
    blendFb.create();
    blendFb.attachTexture( blendTex );
    blendFb.attachTextureStencil(stencilTex);
    blendFb.drawToAll();
    blendFb.check();
    blendFb.unbind();

    areaTex = framebuffer_texture_t(new Texture());
    areaTex->createTexture(AREATEX_WIDTH,AREATEX_HEIGHT,GL_RG,GL_RG8,GL_UNSIGNED_BYTE,areaTexBytes);

    searchTex = framebuffer_texture_t(new Texture());
    searchTex->createTexture(SEARCHTEX_WIDTH,SEARCHTEX_HEIGHT,GL_RED,GL_R8,GL_UNSIGNED_BYTE,searchTexBytes);


    auto qb = TriangleMeshGenerator::createFullScreenQuadMesh();
    qb->createBuffers(quadMesh);

    loadShader();

    assert_no_glerror();

    //    ssaoShader  =  ShaderLoader::instance()->load<SSAOShader>("post_processing/ssao2.glsl");
    //    blurShader = ShaderLoader::instance()->load<MVPTextureShader>("post_processing/ssao_blur.glsl");

    //    setKernelSize(32);

    //    auto randomImage = ImageGenerator::randomNormalized(32,32);
    //    randomTexture = std::make_shared<Texture>();
    //    randomTexture->fromImage(*randomImage);
    //    randomTexture->setWrap(GL_REPEAT);

    //    clearSSAO();
}

void SMAA::loadShader()
{
    //example:
    //#define SMAA_RT_METRICS float4(1.0 / 1280.0, 1.0 / 720.0, 1280.0, 720.0)
    vec4 rtMetrics(1.0f/screenSize.x,1.0f/screenSize.y,screenSize.x,screenSize.y);
    std::string rtMetricsStr = "#define SMAA_RT_METRICS float4("
            + std::to_string(rtMetrics.x) + ","
            + std::to_string(rtMetrics.y) + ","
            + std::to_string(rtMetrics.z) + ","
            + std::to_string(rtMetrics.w) +
            ")";

    std::string qualityStr;
    switch(quality){
    case Quality::SMAA_PRESET_LOW:
        qualityStr = "#define SMAA_PRESET_LOW";
        break;
    case Quality::SMAA_PRESET_MEDIUM:
        qualityStr = "#define SMAA_PRESET_MEDIUM";
        break;
    case Quality::SMAA_PRESET_HIGH:
        qualityStr = "#define SMAA_PRESET_HIGH";
        break;
    case Quality::SMAA_PRESET_ULTRA:
        qualityStr = "#define SMAA_PRESET_ULTRA";
        break;
    }

    ShaderPart::ShaderCodeInjections smaaInjection;
    smaaInjection.emplace_back(GL_VERTEX_SHADER,rtMetricsStr,1);
    smaaInjection.emplace_back(GL_FRAGMENT_SHADER,rtMetricsStr,1);
    smaaInjection.emplace_back(GL_VERTEX_SHADER,qualityStr,2);
    smaaInjection.emplace_back(GL_FRAGMENT_SHADER,qualityStr,2);

    smaaEdgeDetectionShader = ShaderLoader::instance()->load<PostProcessingShader>("post_processing/smaa/SMAAEdgeDetection.glsl",smaaInjection);
    smaaBlendingWeightCalculationShader = ShaderLoader::instance()->load<SMAABlendingWeightCalculationShader>("post_processing/smaa/SMAABlendingWeightCalculation.glsl",smaaInjection);
    smaaNeighborhoodBlendingShader = ShaderLoader::instance()->load<SMAANeighborhoodBlendingShader>("post_processing/smaa/SMAANeighborhoodBlending.glsl",smaaInjection);

    shaderLoaded = true;
    assert_no_glerror();

}

void SMAA::resize(int w, int h)
{
    screenSize = vec2(w,h);
    edgesFb.resize(w,h);
    blendFb.resize(w,h);
    shaderLoaded = false;
}

void SMAA::render(framebuffer_texture_t input, Framebuffer &output)
{
    if(!shaderLoaded)
        loadShader();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glViewport(0, 0, screenSize.x,screenSize.y);

    //write 1 to stencil if the pixel is not discarded
    glStencilFunc(GL_ALWAYS, 0x1, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    edgesFb.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    smaaEdgeDetectionShader->bind();
    smaaEdgeDetectionShader->uploadTexture(input.get() );
    quadMesh.bindAndDraw();
    smaaEdgeDetectionShader->unbind();
    assert_no_glerror();


    //only work on pixels that are marked with 1
    glStencilFunc(GL_EQUAL, 0x1, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


    blendFb.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    smaaBlendingWeightCalculationShader->bind();
    smaaBlendingWeightCalculationShader->uploadTextures(edgesTex.get(),areaTex.get(),searchTex.get());
    quadMesh.bindAndDraw();
    smaaBlendingWeightCalculationShader->unbind();
    assert_no_glerror();


    glDisable(GL_STENCIL_TEST);

    output.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    smaaNeighborhoodBlendingShader->bind();
    smaaNeighborhoodBlendingShader->uploadTextures(input.get(),blendTex.get());
    quadMesh.bindAndDraw();
    smaaNeighborhoodBlendingShader->unbind();
    assert_no_glerror();


    glEnable(GL_DEPTH_TEST);

}



