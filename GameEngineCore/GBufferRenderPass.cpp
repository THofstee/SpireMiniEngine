#include "WorldRenderPass.h"
#include "Material.h"
#include "Mesh.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	class GBufferRenderPass : public WorldRenderPass
	{
	private:
		String shaderEntryPoint = R"(
			shader GBufferPass targets StandardPipeline
			{
				public using VertexAttributes;
				[Binding: "0"]
				public using ForwardBasePassParams;
				[Binding: "3"]
				public using ANIMATION;
				public using TangentSpaceTransform;
				public using MaterialGeometry;
				public using VertexTransform;
				[Binding: "2"]
				public using MaterialPattern;
				vec3 lightParam = vec3(roughness, metallic, specular);
				public out @Fragment vec3 outputAlbedo = albedo;
				public out @Fragment vec3 outputPbr = lightParam;
				public out @Fragment vec3 outputNormal = TangentSpaceToWorldSpace(vec3(normal.x, -normal.y, normal.z)) * 0.5 + 0.5;
			};
		)";
	protected:
		virtual String GetEntryPointShader() override
		{
			return shaderEntryPoint;
		}
		virtual char * GetName() override
		{
			return "GBuffer";
		}
		virtual void Create() override
		{
			renderTargetLayout = hwRenderer->CreateRenderTargetLayout(MakeArray(
				TextureUsage::ColorAttachment, 
				TextureUsage::ColorAttachment,
				TextureUsage::ColorAttachment,
				TextureUsage::DepthAttachment).GetArrayView());
		}
	};

	WorldRenderPass * CreateGBufferRenderPass()
	{
		return new GBufferRenderPass();
	}
}