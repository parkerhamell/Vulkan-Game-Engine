#include "Utilities.h"
#include "../CCL.h"

namespace UTIL
{
	GW::MATH::GVECTORF GetRandomVelocityVector()
	{
		GW::MATH::GVECTORF vel = {float((rand() % 20) - 10), 0.0f, float((rand() % 20) - 10)};
		if (vel.x <= 0.0f && vel.x > -1.0f)
			vel.x = -1.0f;
		else if (vel.x >= 0.0f && vel.x < 1.0f)
			vel.x = 1.0f;

		if (vel.z <= 0.0f && vel.z > -1.0f)
			vel.z = -1.0f;
		else if (vel.z >= 0.0f && vel.z < 1.0f)
			vel.z = 1.0f;

		GW::MATH::GVector::NormalizeF(vel, vel);

		return vel;
	}

	void ModelToDynObj(entt::registry& registry, entt::entity entity, std::string modelName)
	{	
        auto& config = registry.ctx().get<UTIL::Config>();

        auto modelManagerView = registry.view<DRAW::ModelManager>();
        if (modelManagerView.empty())
        {
            std::cout << "ERROR: ModelToDynObj: No Model Manager found.\n";
            return;
        }

        auto& modelManager = registry.get<DRAW::ModelManager>(modelManagerView.front());

        DRAW::MeshCollections source = modelManager.GetCollections(modelName);

        auto& entityTransform = registry.get_or_emplace<GAME::Transform>(entity);
        auto& entityCollection = registry.get_or_emplace<DRAW::MeshCollections>(entity);

        bool hasTransform = false;

        for (auto mesh : source.meshes)
        {
            auto newMesh = registry.create();

            const auto& srcGeo = registry.get<DRAW::GeometryData>(mesh);
            const auto& srcGPU = registry.get<DRAW::GPUInstance>(mesh);

            registry.emplace<DRAW::GeometryData>(newMesh, srcGeo);
            registry.emplace<DRAW::GPUInstance>(newMesh, srcGPU);

            if (!hasTransform)
            {
                entityTransform.matrix = srcGPU.transform;
                hasTransform = true;
            }

            entityCollection.meshes.push_back(newMesh);
        }

        entityCollection.collider = source.collider;

	}

} // namespace UTIL