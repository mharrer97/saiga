

#pragma once

//#include "saiga/core/geometry/triangle_mesh.h"
//#include "saiga/vulkan/Base.h"
//#include "saiga/vulkan/VulkanAsset.h"
//#include "saiga/vulkan/VulkanBuffer.hpp"
//#include "saiga/vulkan/buffer/UniformBuffer.h"
//#include "saiga/vulkan/pipeline/Pipeline.h"
//#include "saiga/vulkan/svulkan.h"
//#include "saiga/vulkan/texture/Texture.h"

#include "saiga/vulkan/renderModules/AssetRenderer.h"
#include "saiga/vulkan/renderModules/LineAssetRenderer.h"
#include "saiga/vulkan/renderModules/PointCloudRenderer.h"
#include "saiga/vulkan/renderModules/TextureDisplay.h"
#include "saiga/vulkan/renderModules/TexturedAssetRenderer.h"

namespace Saiga
{
namespace Vulkan
{
template <typename Deferred, typename Forward>
class SAIGA_VULKAN_API UniversalRenderer
{
   public:
    ~UniversalRenderer() { destroy(); }

    void destroy()
    {
        deferred.destroy();
        forward.destroy();
    }

    void reload()
    {
        deferred.reload();
        forward.reload();
    }

    Deferred deferred;
    Forward forward;
};

using UniversalAssetRenderer         = UniversalRenderer<DeferredAssetRenderer, AssetRenderer>;
using UniversalPointCloudRenderer    = UniversalRenderer<DeferredPointCloudRenderer, PointCloudRenderer>;
using UniversalLineAssetRenderer     = UniversalRenderer<DeferredLineAssetRenderer, LineAssetRenderer>;
using UniversalTexturedAssetRenderer = UniversalRenderer<DeferredTexturedAssetRenderer, TexturedAssetRenderer>;
using UniversalTextureDisplay        = UniversalRenderer<DeferredTextureDisplay, TextureDisplay>;

template <typename Deferred, typename Forward, typename Shadow>
class SAIGA_VULKAN_API UniversalShadowRenderer
{
   public:
    ~UniversalShadowRenderer() { destroy(); }

    void destroy()
    {
        deferred.destroy();
        forward.destroy();
        shadow.destroy();
    }

    void reload()
    {
        deferred.reload();
        forward.reload();
        shadow.reload();
    }

    Deferred deferred;
    Forward forward;
    Shadow shadow;
};

using UniversalShadowAssetRenderer = UniversalShadowRenderer<DeferredAssetRenderer, AssetRenderer, ShadowAssetRenderer>;
using UniversalShadowLineAssetRenderer =
    UniversalShadowRenderer<DeferredLineAssetRenderer, LineAssetRenderer, ShadowLineAssetRenderer>;
using UniversalShadowPointCloudRenderer =
    UniversalShadowRenderer<DeferredPointCloudRenderer, PointCloudRenderer, ShadowPointCloudRenderer>;
using UniversalShadowTexturedAssetRenderer =
    UniversalShadowRenderer<DeferredTexturedAssetRenderer, TexturedAssetRenderer, ShadowAssetRenderer>;

}  // namespace Vulkan
}  // namespace Saiga
