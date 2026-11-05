#include "C:/Users/hype/Documents/GitHub/dev4-starting-materials-v1-1-parkerhamell/ProjectTemplate/build/CMakeFiles/PPIV.dir/Debug/cmake_pch.hxx"
#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"

void DeleteAssets(entt::registry& registry) {
	auto destroyView = registry.view<GAME::ToDestroy>();
	for (auto e : destroyView)
	{
		registry.destroy(e);
	}
}

void GAME::GameManager::OnUpdate(entt::registry& registry, entt::entity entity)
{
	auto gameState = registry.view<GAME::GameOver>();
	for (auto state : gameState)
	{
		if (registry.all_of<GAME::GameOver>(state))
		{
			return;
		}
	}

	auto view = registry.view<GAME::Transform, DRAW::MeshCollections>();
	for (auto entity : view)
	{
		auto& transform = view.get<GAME::Transform>(entity);
		auto& meshCollection = view.get<DRAW::MeshCollections>(entity);

		for (auto meshEntity : meshCollection.meshes)
		{
			if (registry.all_of<DRAW::GPUInstance>(meshEntity))
			{
				auto& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);
				gpuInstance.transform = transform.matrix;
			}
		}
	}

	auto motionView = registry.view<GAME::Transform, GAME::Velocity>();
	for (auto movingEntity : motionView)
	{
		float dt = registry.ctx().get<UTIL::DeltaTime>().dtSec;
		auto& transform = motionView.get<GAME::Transform>(movingEntity);
		auto& velocity = motionView.get<GAME::Velocity>(movingEntity);

		GW::MATH::GVECTORF displacement;
		GW::MATH::GVector::ScaleF(velocity.directionVector, velocity.speed * dt, displacement);

		GW::MATH::GMatrix::TranslateLocalF(transform.matrix, displacement, transform.matrix);
	}

	registry.patch<Player>(registry.view<Player>().front());

	auto collisionView = registry.view<GAME::Transform, DRAW::MeshCollections, GAME::Collidable>();

	for (auto entityCol : collisionView)
	{
		auto& collisionTransform = registry.get<GAME::Transform>(entityCol);
		auto& collisionMeshes = registry.get<DRAW::MeshCollections>(entityCol);

		for (auto entityCheck : collisionView)
		{
			if (entityCheck == entityCol)
				continue;

			GW::MATH::GOBBF collisionCollider = collisionMeshes.collider;
			GW::MATH::GVECTORF collisionScale = { 1.0f, 1.0f, 1.0f };
			GW::MATH::GQUATERNIONF collisionRotation = { 0.0f, 0.0f, 0.0f, 1.0f };

			GW::MATH::GMatrix::GetScaleF(collisionTransform.matrix, collisionScale);

			collisionCollider.extent.x *= collisionScale.x;
			collisionCollider.extent.y *= collisionScale.y;
			collisionCollider.extent.z *= collisionScale.z;

			GW::MATH::GMatrix::VectorXMatrixF(collisionTransform.matrix, collisionCollider.center, collisionCollider.center);
			GW::MATH::GQuaternion::SetByMatrixF(collisionTransform.matrix, collisionRotation);
			GW::MATH::GQuaternion::MultiplyQuaternionF(collisionCollider.rotation, collisionRotation, collisionCollider.rotation);

			auto checkTransform = registry.get<Transform>(entityCheck);
			auto checkMeshes = registry.get<DRAW::MeshCollections>(entityCheck);

			GW::MATH::GVECTORF checkScale = { 1.0f, 1.0f, 1.0f };
			GW::MATH::GOBBF checkCollider = checkMeshes.collider;
			GW::MATH::GQUATERNIONF checkRotation = { 0.0f, 0.0f, 0.0f, 1.0f };

			GW::MATH::GMatrix::GetScaleF(checkTransform.matrix, checkScale);

			checkCollider.extent.x *= checkScale.x;
			checkCollider.extent.y *= checkScale.y;
			checkCollider.extent.z *= checkScale.z;

			GW::MATH::GMatrix::VectorXMatrixF(checkTransform.matrix, checkCollider.center, checkCollider.center);
			GW::MATH::GQuaternion::SetByMatrixF(checkTransform.matrix, checkRotation);
			GW::MATH::GQuaternion::MultiplyQuaternionF(checkCollider.rotation, checkRotation, checkCollider.rotation);


			GW::I::GCollisionImplementation::GCollisionCheck hit = GW::MATH::GCollision::GCollisionCheck::NO_COLLISION;
			GW::I::GCollisionImplementation::TestOBBToOBBF(collisionCollider, checkCollider, hit);

			if (hit == GW::I::GCollisionImplementation::GCollisionCheck::COLLISION)
			{
				auto bullet = registry.all_of<GAME::Bullet>(entityCol);
				auto wall = registry.all_of<Collidable, Obstacle>(entityCheck);

				if (bullet && wall)
				{
					registry.emplace<GAME::ToDestroy>(entityCol);

					return;
				}

				auto enemy = registry.all_of<Collidable, GAME::Enemy>(entityCol);
				if (enemy && wall)
				{
					auto& velocity = registry.get<Velocity>(entityCol);
					GW::MATH::GVECTORF normVector = { 0.0f, 0.0f, 0.0f , 0.0f };
					GW::MATH::GCollision::ClosestPointToOBBF(checkCollider, collisionCollider.center, normVector);

					GW::MATH::GVector::SubtractVectorF(collisionCollider.center, normVector, normVector);
					normVector.y = 0;
					normVector.w = 0;
					GW::MATH::GVector::NormalizeF(normVector, normVector);

					float vDotN = 0.0f;
					GW::MATH::GVector::DotF(velocity.directionVector, normVector, vDotN);
					GW::MATH::GVector::ScaleF(normVector, 2.0f * vDotN, normVector);
					GW::MATH::GVector::SubtractVectorF(velocity.directionVector, normVector, velocity.directionVector);

					return;
				}

				auto player = registry.all_of<GAME::Player>(entityCheck);
				if (enemy && player)
				{
					if (!registry.all_of<GAME::Invulnerable>(entityCheck))
					{
						auto& hp = registry.get<GAME::Health>(entityCheck);
						hp.health -= 1;

						std::cout << "Player health: " << hp.health << std::endl;

						std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

						float invulnCooldown = (*config).at("Player").at("invulnPeriod").as<float>();

						registry.emplace<GAME::Invulnerable>(entityCheck, invulnCooldown);
					}

					return;
				}

				bullet = registry.all_of<GAME::Bullet>(entityCheck);

				if (bullet && enemy)
				{
					auto& hp = registry.get<GAME::Health>(entityCol);

					hp.health -= 1;

					//if (hp.health >= 0)
					//	std::cout << "Enemy health: " << hp.health << std::endl;

					registry.emplace_or_replace<GAME::ToDestroy>(entityCheck);

					return;
				}
				


			}
			DeleteAssets(registry);

		}
	}

	auto enemyView = registry.view<GAME::Enemy, GAME::Health>();
	for (auto enemyEntity : enemyView)
	{
		auto& hp = registry.get<GAME::Health>(enemyEntity);

		if (hp.health > 0)
			continue;

		registry.emplace<ToDestroy>(enemyEntity);

		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

		auto& shatter = registry.get<GAME::Shatters>(enemyEntity);
		int shatterCount = 0;

		float shatterScale = (*config).at("Enemy1").at("shatterScale").as<float>();

		bool canShatter = registry.all_of<GAME::Shatters>(enemyEntity);

		if (canShatter)
		{
			shatter.remainingShatters = shatter.remainingShatters - 1;
			shatterCount = shatter.remainingShatters;
		}

		if (canShatter)
		{
			auto& parentTransform = registry.get<GAME::Transform>(enemyEntity);

			for (int i = 0; i < shatterCount; i++)
			{
				std::string enemyModelName = (*config).at("Enemy1").at("model").as<std::string>();

				auto newEnemy = registry.create();

				float enemySpeed = (*config).at("Enemy1").at("speed").as<float>();
				int enemyHealth = (*config).at("Enemy1").at("hitpoints").as<float>();
				auto& enemyTransform = registry.emplace<GAME::Transform>(newEnemy);
				enemyTransform.matrix = parentTransform.matrix;

				registry.emplace<GAME::Enemy>(newEnemy);
				registry.emplace<GAME::Collidable>(newEnemy);
				registry.emplace<GAME::Health>(newEnemy, enemyHealth);
				registry.emplace<GAME::Shatters>(newEnemy, shatterCount);

				UTIL::ModelToDynObj(registry, newEnemy, enemyModelName);

				GW::MATH::GVECTORF enemyDirection = { 0, 0, 0, 0 };

				enemyDirection = UTIL::GetRandomVelocityVector();

				GAME::Velocity enemyVelocity;
				enemyVelocity.directionVector = enemyDirection;
				enemyVelocity.speed = enemySpeed;
				registry.emplace<GAME::Velocity>(newEnemy, enemyVelocity);

				GW::MATH::GVECTORF enemyScaleVector = { 1.0f, 1.0f, 1.0f, 1.0f };

				GW::MATH::GVector::ScaleF(enemyScaleVector, shatterScale, enemyScaleVector);
				GW::MATH::GMatrix::ScaleGlobalF(enemyTransform.matrix, enemyScaleVector, enemyTransform.matrix);

				//std::cout << "Entity " << i << std::endl;
			}
		}


	}

	if (registry.view<GAME::Enemy>().begin() == registry.view<GAME::Enemy>().end())
	{
		auto gameOver = registry.create();
		registry.emplace<GAME::GameOver>(gameOver);
		std::cout << "You win, good job!" << std::endl;
		return;
	}

	auto playerView = registry.view<GAME::Player, GAME::Health>();

	if (playerView.begin() != playerView.end())
	{
		bool allPlayersDead = false;

		for (auto playerEntity : playerView)
		{
			auto& hp = registry.get<GAME::Health>(playerEntity);

			if (hp.health <= 0)
			{
				allPlayersDead = true;

				//std::cout << "Player has no health" << std::endl;

				break;
			}

		}

		if (allPlayersDead)
		{
			auto gameOver = registry.create();
			registry.emplace<GAME::GameOver>(gameOver);
			std::cout << "You lose, game over" << std::endl;
			return;
		}
	}

	DeleteAssets(registry);
}