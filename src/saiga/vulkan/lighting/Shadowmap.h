/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#pragma once

//#include "saiga/opengl/framebuffer.h"
//#include "saiga/opengl/texture/arrayTexture.h"
#include "saiga/core/util/assert.h"
#include "saiga/vulkan/buffer/DepthBuffer.h"
#include "saiga/vulkan/buffer/Framebuffer.h"
#include "saiga/vulkan/buffer/ShadowBuffer.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
enum class ShadowQuality
{
    LOW,     // 16 uint
    MEDIUM,  // 32 uint
    HIGH     // 32 bit float
};


class SAIGA_VULKAN_API ShadowmapBase
{
   protected:
    int w, h;

   public:
    ivec2 getSize() { return ivec2(w, h); }
    // void bindFramebuffer();
    // void unbindFramebuffer();
    Framebuffer frameBuffer;
};

/**
 * Simple shadow map with one 2D depth texture.
 * Used by box- and spotlight
 */
class SAIGA_VULKAN_API SimpleShadowmap : public ShadowmapBase
{
    std::shared_ptr<ShadowBuffer> shadowBuffer;

   public:
    SimpleShadowmap();
    void init(VulkanBase& base, int w, int h,
              vk::RenderPass shadowPass);  //, ShadowQuality quality = ShadowQuality::LOW);
    ~SimpleShadowmap() { shadowBuffer->destroy(); }
    std::shared_ptr<ShadowBuffer> getShadowBuffer() { return shadowBuffer; }
};

/**
 * Cube shadow map with one cube depth texture.
 * Used by point light
 */
/*class SAIGA_OPENGL_API CubeShadowmap : public ShadowmapBase
{
    std::shared_ptr<raw_Texture> depthTexture;

   public:
    CubeShadowmap(int w, int h, ShadowQuality quality = ShadowQuality::LOW);
    ~CubeShadowmap() {}
    std::shared_ptr<raw_Texture> getDepthTexture() { return depthTexture; }
    void bindCubeFace(GLenum side);
};
*/
/**
 * Cascaded shadow map with numCascades depth textures.
 * Used by directional light
 */
/*class SAIGA_OPENGL_API CascadedShadowmap : public ShadowmapBase
{
    //    std::vector<std::shared_ptr<raw_Texture>> depthTextures;
    std::shared_ptr<ArrayTexture2D> depthTexture;

   public:
    CascadedShadowmap(int w, int h, int numCascades, ShadowQuality quality = ShadowQuality::LOW);
    ~CascadedShadowmap() {}

    //    std::shared_ptr<raw_Texture> getDepthTexture(unsigned int n){
    //        SAIGA_ASSERT(n < depthTextures.size());
    //        return depthTextures[n];
    //    }
    //    std::vector<std::shared_ptr<raw_Texture>>& getDepthTextures(){ return depthTextures;}

    std::shared_ptr<ArrayTexture2D> getDepthTexture() { return depthTexture; }

    void bindAttachCascade(int n);
};
*/

}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
