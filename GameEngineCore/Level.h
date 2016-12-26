#ifndef GAME_ENGINE_LEVEL_H
#define GAME_ENGINE_LEVEL_H

#include "CoreLib/Basic.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "StaticMeshActor.h"
#include "CameraActor.h"
#include "Material.h"
#include "MotionGraph.h"
#include "RenderContext.h"

namespace GameEngine
{
	class Level : public CoreLib::Object
	{
	public:
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Material>> Materials;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Mesh>> Meshes;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Skeleton>> Skeletons;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<SkeletalAnimation>> Animations;
        CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<MotionGraph>> MotionGraphs;
		CoreLib::EnumerableDictionary<CoreLib::String, RefPtr<Actor>> Actors;
		CoreLib::RefPtr<CameraActor> CurrentCamera;
		Level(const CoreLib::String & fileName);
		~Level();
		Mesh * LoadMesh(CoreLib::String fileName);
		Mesh * LoadMesh(CoreLib::String name, Mesh m);
		Skeleton * LoadSkeleton(const CoreLib::String & fileName);
		Material * LoadMaterial(const CoreLib::String & fileName);
		Material * CreateNewMaterial();
		SkeletalAnimation * LoadSkeletalAnimation(const CoreLib::String & fileName);
        MotionGraph * LoadMotionGraph(const CoreLib::String & fileName);
		Actor * FindActor(const CoreLib::String & name);
		void RegisterActor(Actor * actor);
		void UnregisterActor(Actor * actor);
	};
}

#endif