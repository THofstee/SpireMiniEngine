#include "SkeletalMeshActor.h"
#include "Engine.h"

namespace GameEngine
{
	bool SkeletalMeshActor::ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("mesh"))
		{
			parser.ReadToken();
			MeshName = parser.ReadStringLiteral();
			Mesh = level->LoadMesh(MeshName);
			Bounds = Mesh->Bounds;
			if (!Mesh)
				isInvalid = false;
			return true;
		}
		if (parser.LookAhead("material"))
		{
			if (parser.NextToken(1).Content == "{")
			{
				MaterialInstance = level->CreateNewMaterial();
				MaterialInstance->Parse(parser);
				MaterialInstance->Name = Name;
			}
			else
			{
				parser.ReadToken();
				auto materialName = parser.ReadStringLiteral();
				MaterialInstance = level->LoadMaterial(materialName);
				if (!MaterialInstance)
					isInvalid = true;
			}
			return true;
		}
		if (parser.LookAhead("Skeleton"))
		{
			parser.ReadToken();
			SkeletonName = parser.ReadStringLiteral();
			Skeleton = level->LoadSkeleton(SkeletonName);
			if (!Skeleton)
				isInvalid = true;
			return true;
		}
		if (parser.LookAhead("SimpleAnimation"))
		{
			parser.ReadToken();
			SimpleAnimationName = parser.ReadStringLiteral();
			SimpleAnimation = level->LoadSkeletalAnimation(SimpleAnimationName);
			if (!SimpleAnimation)
				isInvalid = true;
			return true;
		}
		return false;
	}

	void GameEngine::SkeletalMeshActor::Tick()
	{
		auto time = Engine::Instance()->GetTime();
        if (Animation)
			Animation->GetPose(nextPose, time);
	}

	void SkeletalMeshActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (!drawable)
			drawable = params.rendererService->CreateSkeletalDrawable(Mesh, Skeleton, MaterialInstance);
		drawable->UpdateTransformUniform(localTransform, nextPose);
		params.sink->AddDrawable(drawable.Ptr());
	}

	void SkeletalMeshActor::OnLoad()
	{
        if (this->SimpleAnimation)
            Animation = new SimpleAnimationSynthesizer(Skeleton, this->SimpleAnimation);
		Tick();
	}

	void SkeletalMeshActor::SetLocalTransform(const VectorMath::Matrix4 & val)
	{
		Actor::SetLocalTransform(val);
		CoreLib::Graphics::TransformBBox(Bounds, localTransform, Mesh->Bounds);
	}
}