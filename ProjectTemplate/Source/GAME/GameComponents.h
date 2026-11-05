#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
	///*** Tags ***///
	struct Player { static void UpdatePlayer(entt::registry& registry, entt::entity entity); };

	struct Enemy {};
	
	struct Bullet {};

	struct Collidable {};

	struct Health { int health; };

	struct Obstacle {};

	struct Shatters { int remainingShatters; };

	struct Invulnerable { float cooldown; };

	struct ToDestroy {};

	struct Firing { float cooldown; };

	struct GameOver {};

	///*** Components ***///
	struct Transform { GW::MATH::GMATRIXF matrix; };

	struct Velocity { float speed = 1.0f; GW::MATH::GVECTORF directionVector{1, 0, 0}; };

	struct GameManager { static void OnUpdate(entt::registry& registry, entt::entity entity); };

}// namespace GAME
#endif // !GAME_COMPONENTS_H_