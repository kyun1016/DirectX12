#pragma once

#include "../DonutCore/DonutCorePch.h"
#include "../DonutCore/Math.h"

#include "../nvrhi/nvrhi.h"
#include "../nvrhi/misc.h"
#include "../nvrhi/utils.h"

#define CGLTF_IMPLEMENTATION
#include "../cgltf/cgltf.h"

#include "../json/value.h"

#include "../ShaderMake/ShaderBlob.h"

#include "../stb/stb_image.h"
#include "../stb/stb_image_write.h"

#include "../DonutEngine\View.h"
#include "../DonutEngine\AudioCache.h"
#include "../DonutEngine\AudioEngine.h"
#include "../DonutEngine\BindingCache.h"
#include "../DonutEngine\CommonRenderPasses.h"
#include "../DonutEngine\ConsoleInterpreter.h"
#include "../DonutEngine\ConsoleObjects.h"
#include "../DonutEngine\dds.h"
#include "../DonutEngine\DDSFile.h"
#include "../DonutEngine\DescriptorTableManager.h"
#include "../DonutEngine\FramebufferFactory.h"
#include "../DonutEngine\GltfImporter.h"
#include "../DonutEngine\IesProfile.h"
#include "../DonutEngine\KeyframeAnimation.h"
#include "../DonutEngine\MaterialBindingCache.h"
#include "../DonutEngine\Scene.h"
#include "../DonutEngine\SceneGraph.h"
#include "../DonutEngine\SceneTypes.h"
#include "../DonutEngine\ShaderFactory.h"
#include "../DonutEngine\ShadowMap.h"
#include "../DonutEngine\TextureCache.h"

using namespace donut::math;
#include "../DonutShaders/DonutShadersPch.h"