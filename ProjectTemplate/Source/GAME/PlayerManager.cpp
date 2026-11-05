#include "C:/Users/hype/Documents/GitHub/dev4-starting-materials-v1-1-parkerhamell/ProjectTemplate/build/CMakeFiles/PPIV.dir/Debug/cmake_pch.hxx"
#include "GameComponents.h"
#include "../UTIL/Utilities.h"
#include "../DRAW/DrawComponents.h"

void GAME::Player::UpdatePlayer(entt::registry& registry, entt::entity entity)
{
	GW::SYSTEM::GLog log;

	// Player
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
	float speed = (*config).at("Player").at("speed").as<float>();
	float fireRate = (*config).at("Player").at("firerate").as<float>();
	const auto& deltaTime = registry.ctx().emplace<UTIL::DeltaTime>().dtSec;
	auto& transform = registry.get<GAME::Transform>(entity);
	auto& input = registry.ctx().get<UTIL::Input>();

	// Bullet
	auto modelManagerView = registry.view<DRAW::ModelManager>();
	float bulletSpeed = (*config).at("Bullet").at("speed").as<float>();

	if (modelManagerView.empty())
		log.LogCategorized("ERROR", "GameplayBehavior: No Model Manager found. Check OnConstruct_GPULevel.");

	auto& modelManager = registry.get<DRAW::ModelManager>(modelManagerView.front());

	GW::MATH::GVECTORF position{ 0.0f, 0.0f, 0.0f, 0.0f };

	float w, a, s, d;
	input.immediateInput.GetState(G_KEY_W, w);
	input.immediateInput.GetState(G_KEY_A, a);
	input.immediateInput.GetState(G_KEY_S, s);
	input.immediateInput.GetState(G_KEY_D, d);

	position.x += d - a;
	position.z += w - s;

	if (position.x != 0 || position.z != 0)
		GW::MATH::GVector::NormalizeF(position, position);

	float scalar = speed * static_cast<float>(deltaTime);

	GW::MATH::GVector::ScaleF(position, scalar, position);

	GW::MATH::GMATRIXF translateMatrix;
	GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, position, translateMatrix);
	transform.matrix = translateMatrix;
	//std::string debugInput = "W: " + std::to_string(w) + " " + "A: " + std::to_string(a) + " " + "S: " + std::to_string(s) + " " + "D: " + std::to_string(d);

	//std::string debugSpeed = "Player Speed: " + std::to_string(speed);

	//std::cout << debugInput << std::endl;

	if (registry.all_of<GAME::Invulnerable>(entity))
	{
		auto& inv = registry.get<GAME::Invulnerable>(entity);
		inv.cooldown -= deltaTime;

		if (inv.cooldown <= 0.0f)
		{
			registry.remove<GAME::Invulnerable>(entity);
			std::cout << "Invulnerability Removed" << std::endl;
		}
			
	}

	if (registry.all_of<Firing>(entity))
	{
		auto& firing = registry.get<GAME::Firing>(entity);
		firing.cooldown -= deltaTime;

		if (firing.cooldown <= 0.0f)
			registry.remove<GAME::Firing>(entity);
	}
	else
	{
		float left, right, down, up;
		input.immediateInput.GetState(G_KEY_LEFT, left);
		input.immediateInput.GetState(G_KEY_RIGHT, right);
		input.immediateInput.GetState(G_KEY_DOWN, down);
		input.immediateInput.GetState(G_KEY_UP, up);

		if (left > 0.f || right > 0.f || up > 0.f || down > 0.f)
		{
			auto bulletEntity = registry.create();
			registry.emplace<GAME::Collidable>(bulletEntity);
			registry.emplace<GAME::Bullet>(bulletEntity);
			auto& bulletTransform = registry.emplace<GAME::Transform>(bulletEntity);
			auto& bulletCollection = registry.emplace<DRAW::MeshCollections>(bulletEntity);
			
			std::string bulletModelName = (*config).at("Bullet").at("model").as<std::string>();
			DRAW::MeshCollections bulletMeshSource = modelManager.GetCollections(bulletModelName);
			bulletCollection.collider = bulletMeshSource.collider;

			for (auto mesh : bulletMeshSource.meshes)
			{
				auto newMesh = registry.create();

				const auto& srcGeo = registry.get<DRAW::GeometryData>(mesh);
				const auto& srcGPU = registry.get<DRAW::GPUInstance>(mesh);
				registry.emplace<DRAW::GeometryData>(newMesh, srcGeo);
				registry.emplace<DRAW::GPUInstance>(newMesh, srcGPU);

				if (bulletCollection.meshes.empty())
					bulletTransform.matrix = translateMatrix;

				bulletCollection.meshes.push_back(newMesh);
			}

			GW::MATH::GVECTORF bulletDirection{ 0.0f, 0.0f, 0.0f, 0.0f };

			bulletDirection.x += right - left;
			bulletDirection.z += up - down;

			if (bulletDirection.x != 0 || bulletDirection.z != 0)
				GW::MATH::GVector::NormalizeF(bulletDirection, bulletDirection);

			GAME::Velocity bulletVelocity;
			bulletVelocity.directionVector = bulletDirection;
			bulletVelocity.speed = bulletSpeed;
			registry.emplace<GAME::Velocity>(bulletEntity, bulletVelocity);

			GAME::Firing firingComp;
			firingComp.cooldown = fireRate;

			registry.emplace<Firing>(entity, firingComp);
		}
	}



}
