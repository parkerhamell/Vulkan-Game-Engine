#include "DrawComponents.h"
#include "../CCL.h"
#include "../GAME/GameComponents.h"
namespace DRAW
{
	//*** HELPERS ***//
	void AddSceneData(entt::registry& registry, entt::entity entity)
	{
		auto& sceneData = registry.emplace<SceneData>(entity,
			SceneData{ { -1.0f, -1.0f, 2.0f }, { 0.9f, 0.9f, 0.9f }, { 0.2f, 0.2f, 0.2f } });

		GW::MATH::GVector::NormalizeF(sceneData.sunDirection, sceneData.sunDirection);
		auto& renderer = registry.get<VulkanRenderer>(entity);
		sceneData.projectionMatrix = renderer.projMatrix;
	}

	void OnConstruct_CPULevel(entt::registry& registry, entt::entity entity)
	{
		CPULevel& cpuLevel = registry.get<CPULevel>(entity);

		GW::SYSTEM::GLog log;
		log.Create("CPULevelLog");
		log.EnableConsoleLogging(true);

		bool loadLevel = cpuLevel.level.LoadLevel(cpuLevel.levelJsonPath.c_str(), cpuLevel.h2bFolderPath.c_str(), log);

		if (!loadLevel)
			log.LogCategorized("ERROR", "OnConstruct_CPULevel: Failed To Load Level.");

		log.LogCategorized("DRAW", "OnConstruct_CPULevel: Level loaded successfully");
	}

	void OnConstruct_GPULevel(entt::registry& registry, entt::entity entity)
	{
		CPULevel* cpuLevel = registry.try_get<CPULevel>(entity);

		GW::SYSTEM::GLog log;
		log.Create("GPULevelLog");
		log.EnableConsoleLogging(true);

		if (!cpuLevel)
		{
			log.LogCategorized("ERROR", "OnConstruct_GPULevel: CPULevel is null");
			return;
		}

		registry.emplace<VulkanVertexBuffer>(entity);
		registry.emplace<std::vector<H2B::VERTEX>>(entity, cpuLevel->level.levelVertices);
		registry.patch<VulkanVertexBuffer>(entity);

		registry.emplace<VulkanIndexBuffer>(entity);
		registry.emplace<std::vector<unsigned int>>(entity, cpuLevel->level.levelIndices);
		registry.patch<VulkanIndexBuffer>(entity);

		log.LogCategorized("DRAW", "Construct_GPULevel: Vertex and index buffers emplaced and patched");

		const std::vector<Level_Data::BLENDER_OBJECT>& blenderObjects = cpuLevel->level.blenderObjects;
		const std::vector<Level_Data::LEVEL_MODEL>& levelModels = cpuLevel->level.levelModels;
		const std::vector<H2B::MESH>& levelMeshes = cpuLevel->level.levelMeshes;
		const std::vector<GW::MATH::GMATRIXF>& levelTransforms = cpuLevel->level.levelTransforms;
		const std::vector<H2B::MATERIAL>               levelMaterials = cpuLevel->level.levelMaterials;
		const std::vector<GW::MATH::GOBBF>             levelColliders = cpuLevel->level.levelColliders;

		auto modelManagerEntity = registry.create();
		auto& modelManager = registry.emplace<ModelManager>(modelManagerEntity);

		for (const auto& blenderObjs : cpuLevel->level.blenderObjects)
		{
			MeshCollections meshCollections;

			for (int meshIdx = 0; meshIdx < levelModels[blenderObjs.modelIndex].meshCount; meshIdx++)
			{
				auto meshEntity = registry.create();
				GeometryData meshGeo{};
				meshGeo.indexStart = levelMeshes[levelModels[blenderObjs.modelIndex].meshStart + meshIdx].drawInfo.indexOffset + levelModels[blenderObjs.modelIndex].indexStart;
				meshGeo.indexCount = levelMeshes[levelModels[blenderObjs.modelIndex].meshStart + meshIdx].drawInfo.indexCount;
				meshGeo.vertexStart = levelModels[blenderObjs.modelIndex].vertexStart;
				registry.emplace<GeometryData>(meshEntity, meshGeo);

				GPUInstance instance{};
				instance.transform = levelTransforms[blenderObjs.transformIndex];
				instance.matData = levelMaterials[levelModels[blenderObjs.modelIndex].materialStart + meshIdx].attrib;
				registry.emplace<GPUInstance>(meshEntity, instance);

				if (levelModels[blenderObjs.modelIndex].isModelDynamic)
				{
					registry.emplace<DoNotRender>(meshEntity);
					meshCollections.meshes.push_back(meshEntity);
				}

				meshCollections.collider = levelColliders[levelModels[blenderObjs.modelIndex].colliderIndex];

				std::cout << "Level Model: " << blenderObjs.blendername << " Collider Index: " << std::to_string(levelModels[blenderObjs.modelIndex].colliderIndex) << std::endl;

				if (levelModels[blenderObjs.modelIndex].isCollidable)
				{
					auto collideEntity = registry.create();
					registry.emplace<GAME::Transform>(collideEntity, GAME::Transform{ levelTransforms[blenderObjs.transformIndex] });
					registry.emplace<MeshCollections>(collideEntity, meshCollections);
					registry.emplace<GAME::Collidable>(collideEntity);
					registry.emplace<GAME::Obstacle>(collideEntity);
				}

			}

			if (levelModels[blenderObjs.modelIndex].isModelDynamic)
			{
				modelManager.collections.emplace(blenderObjs.blendername, meshCollections);
			}
		}

		// Test mesh collections and see if theyre retaining information.
		for (const auto& meshCollects : modelManager.collections)
		{
			log.LogCategorized("DEBUG", meshCollects.first.c_str());
		}

		log.LogCategorized("DRAW", "Construct_GPULevel: Created mesh entities with GeometryData & GPUInstance");
	}

	//*** SYSTEMS ***//
	// Forward Declare
	void Destroy_VulkanVertexBuffer(entt::registry& registry, entt::entity entity);

	void Update_VulkanVertexBuffer(entt::registry& registry, entt::entity entity) {
		auto& vertex_buffer = registry.get<VulkanVertexBuffer>(entity);
		// upload the buffer to the GPU
		if (registry.all_of<VulkanRenderer, std::vector<H2B::VERTEX>>(entity)) {
			// if there is already a vertex buffer attached, lets delete it
			if (vertex_buffer.buffer != VK_NULL_HANDLE)
				Destroy_VulkanVertexBuffer(registry, entity);
			// if there is a cpu buffer attached, lets upload it to the GPU then delete it
			auto& vkRenderer = registry.get<VulkanRenderer>(entity);
			auto& vertex_data = registry.get<std::vector<H2B::VERTEX>>(entity);
			// Transfer triangle data to the vertex buffer. (staging would be preferred here)
			GvkHelper::create_buffer(vkRenderer.physicalDevice, vkRenderer.device, sizeof(H2B::VERTEX) * vertex_data.size(),
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertex_buffer.buffer, &vertex_buffer.memory);
			GvkHelper::write_to_buffer(vkRenderer.device, vertex_buffer.memory,
				vertex_data.data(), sizeof(H2B::VERTEX) * vertex_data.size());
			// remove the Vertex Data
			vkDeviceWaitIdle(vkRenderer.device);
			registry.remove<std::vector<H2B::VERTEX>>(entity);
		}
	}

	void Destroy_VulkanVertexBuffer(entt::registry& registry, entt::entity entity) {
		// check if the buffer is allocated, if so, release it
		if (registry.all_of<VulkanVertexBuffer, VulkanRenderer>(entity)) {

			auto& vkRenderer = registry.get<VulkanRenderer>(entity);
			auto& vertex_buffer = registry.get<VulkanVertexBuffer>(entity);
			vkDeviceWaitIdle(vkRenderer.device);
			// Release allocated buffers, shaders & pipeline
			vkDestroyBuffer(vkRenderer.device, vertex_buffer.buffer, nullptr);
			vkFreeMemory(vkRenderer.device, vertex_buffer.memory, nullptr);
		}

	}

	// Forward declare
	void Destroy_VulkanIndexBuffer(entt::registry& registry, entt::entity entity);

	void Update_VulkanIndexBuffer(entt::registry& registry, entt::entity entity) {
		auto& index_buffer = registry.get<VulkanIndexBuffer>(entity);
		// upload the buffer to the GPU
		if (registry.all_of<VulkanRenderer, std::vector<unsigned int>>(entity)) {
			// if there is already a gpu buffer attached, lets delete it
			if (index_buffer.buffer != VK_NULL_HANDLE)
				Destroy_VulkanIndexBuffer(registry, entity);
			// if there is index data attached, lets upload it to the GPU then delete it
			auto& vkRenderer = registry.get<VulkanRenderer>(entity);
			auto& index_data = registry.get<std::vector<unsigned int>>(entity);

			// Transfer triangle data to the vertex buffer. (staging would be preferred here)
			GvkHelper::create_buffer(vkRenderer.physicalDevice, vkRenderer.device, sizeof(unsigned int) * index_data.size(),
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &index_buffer.buffer, &index_buffer.memory);
			GvkHelper::write_to_buffer(vkRenderer.device, index_buffer.memory,
				index_data.data(), sizeof(unsigned int) * index_data.size());
			// remove the index data
			vkDeviceWaitIdle(vkRenderer.device);
			registry.remove<std::vector<unsigned int>>(entity);
		}
	}

	void Destroy_VulkanIndexBuffer(entt::registry& registry, entt::entity entity) {
		// check if the buffer is allocated, if so, release it
		if (registry.all_of<VulkanIndexBuffer, VulkanRenderer>(entity)) {

			auto& vkRenderer = registry.get<VulkanRenderer>(entity);
			auto& index_buffer = registry.get<VulkanIndexBuffer>(entity);

			vkDeviceWaitIdle(vkRenderer.device);
			// Release allocated buffers, shaders & pipeline
			vkDestroyBuffer(vkRenderer.device, index_buffer.buffer, nullptr);
			vkFreeMemory(vkRenderer.device, index_buffer.memory, nullptr);
		}
	}

	void Construct_VulkanGPUInstanceBuffer(entt::registry& registry, entt::entity entity) {
		auto& bufferComponent = registry.get<VulkanGPUInstanceBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		unsigned int frameCount;
		renderer.vlkSurface.GetSwapchainImageCount(frameCount);
		bufferComponent.memory.resize(frameCount);
		bufferComponent.buffer.resize(frameCount);

		for (unsigned int i = 0; i < frameCount; i++)
		{
			GvkHelper::create_buffer(renderer.physicalDevice, renderer.device, sizeof(GPUInstance) * bufferComponent.element_count,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bufferComponent.buffer[i], &bufferComponent.memory[i]);
		}

	}

	// Forward declare
	void Destroy_VulkanGPUInstanceBuffer(entt::registry& registry, entt::entity entity);

	void Update_VulkanGPUInstanceBuffer(entt::registry& registry, entt::entity entity) {
		if (!registry.all_of<std::vector<GPUInstance>>(entity))
			return; // No Instances, so nothing to write. Bail

		auto& gpuBuffer = registry.get<VulkanGPUInstanceBuffer>(entity);
		auto& instances = registry.get<std::vector<GPUInstance>>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		// Resize buffer if needed
		if (instances.size() > gpuBuffer.element_count)
		{
			Destroy_VulkanGPUInstanceBuffer(registry, entity);

			while (instances.size() > gpuBuffer.element_count)
			{
				gpuBuffer.element_count *= 2; // Double the storage size if we ran out
			}

			for (unsigned int i = 0; i < gpuBuffer.buffer.size(); i++)
			{
				GvkHelper::create_buffer(renderer.physicalDevice, renderer.device, sizeof(GPUInstance) * gpuBuffer.element_count,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &gpuBuffer.buffer[i], &gpuBuffer.memory[i]);

				VkDescriptorBufferInfo storageBufferInfo = { gpuBuffer.buffer[i], 0, VK_WHOLE_SIZE };
				VkWriteDescriptorSet storageWrite = {};
				storageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				storageWrite.dstSet = renderer.descriptorSets[i];
				storageWrite.dstBinding = 1; // 1 For the storage buffer
				storageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				storageWrite.descriptorCount = 1;
				storageWrite.pBufferInfo = &storageBufferInfo;

				vkUpdateDescriptorSets(renderer.device, 1, &storageWrite, 0, nullptr);
			}
		}

		if (instances.size() > 0)
		{
			unsigned int sizeInBytes = sizeof(GPUInstance) * instances.size();
			unsigned int frame;
			renderer.vlkSurface.GetSwapchainCurrentImage(frame);
			vkDeviceWaitIdle(renderer.device);
			GvkHelper::write_to_buffer(renderer.device, gpuBuffer.memory[frame], instances.data(), sizeInBytes);
		}

		vkDeviceWaitIdle(renderer.device);
		registry.remove<std::vector<GPUInstance>>(entity);
	}

	void Destroy_VulkanGPUInstanceBuffer(entt::registry& registry, entt::entity entity) {
		auto& gpuBuffer = registry.get<VulkanGPUInstanceBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		vkDeviceWaitIdle(renderer.device);
		for (auto handle : gpuBuffer.buffer)
		{
			vkDestroyBuffer(renderer.device, handle, nullptr);
		}
		for (auto data : gpuBuffer.memory)
		{
			vkFreeMemory(renderer.device, data, nullptr);
		}
	}

	void Construct_VulkanUniformBuffer(entt::registry& registry, entt::entity entity) {

		auto& bufferComponent = registry.get<VulkanUniformBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		// If there isn't a SceneData, add one
		if (!registry.all_of<SceneData>(entity))
		{
			AddSceneData(registry, entity);
		}

		unsigned int frameCount;
		renderer.vlkSurface.GetSwapchainImageCount(frameCount);
		bufferComponent.memory.resize(frameCount);
		bufferComponent.buffer.resize(frameCount);

		for (unsigned int i = 0; i < frameCount; i++)
		{
			GvkHelper::create_buffer(renderer.physicalDevice, renderer.device, sizeof(SceneData),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bufferComponent.buffer[i], &bufferComponent.memory[i]);
		}
	}

	void Update_VulkanUniformBuffer(entt::registry& registry, entt::entity entity) {
		auto& gpuBuffer = registry.get<VulkanUniformBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);
		auto& data = registry.get<SceneData>(entity);

		// TODO : Update Dynamic parts of the Scene Data here
		// We really only support one camera, so use the first one
		auto& camera = registry.get<Camera>(registry.view<Camera>().front());

		data.camPos = camera.camMatrix.row4;
		GW::MATH::GMatrix::InverseF(camera.camMatrix, data.viewMatrix);

		unsigned int frame;
		renderer.vlkSurface.GetSwapchainCurrentImage(frame);
		GvkHelper::write_to_buffer(renderer.device, gpuBuffer.memory[frame], &data, sizeof(SceneData));
	}

	void Destroy_VulkanUniformBuffer(entt::registry& registry, entt::entity entity) {
		auto& gpuBuffer = registry.get<VulkanUniformBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		for (auto handle : gpuBuffer.buffer)
		{
			vkDestroyBuffer(renderer.device, handle, nullptr);
		}
		for (auto data : gpuBuffer.memory)
		{
			vkFreeMemory(renderer.device, data, nullptr);
		}
	}

	void Destroy_MeshCollections(entt::registry& registry, entt::entity entity)
	{
		auto& collection = registry.get<MeshCollections>(entity);
		for (auto meshEnt : collection.meshes)
		{
			if (registry.valid(meshEnt))
				registry.destroy(meshEnt);
		}
	}

	// Use this MACRO to connect the EnTT Component Logic
	CONNECT_COMPONENT_LOGIC() {
		// register the Window component's logic
		registry.on_update<VulkanVertexBuffer>().connect<Update_VulkanVertexBuffer>();
		registry.on_destroy<VulkanVertexBuffer>().connect<Destroy_VulkanVertexBuffer>();

		registry.on_update<VulkanIndexBuffer>().connect<Update_VulkanIndexBuffer>();
		registry.on_destroy<VulkanIndexBuffer>().connect<Destroy_VulkanIndexBuffer>();

		registry.on_construct<VulkanGPUInstanceBuffer>().connect<Construct_VulkanGPUInstanceBuffer>();
		registry.on_update<VulkanGPUInstanceBuffer>().connect<Update_VulkanGPUInstanceBuffer>();
		registry.on_destroy<VulkanGPUInstanceBuffer>().connect<Destroy_VulkanGPUInstanceBuffer>();

		registry.on_construct<VulkanUniformBuffer>().connect<Construct_VulkanUniformBuffer>();
		registry.on_update<VulkanUniformBuffer>().connect<Update_VulkanUniformBuffer>();
		registry.on_destroy<VulkanUniformBuffer>().connect<Destroy_VulkanUniformBuffer>();

		registry.on_construct<CPULevel>().connect<OnConstruct_CPULevel>();
		registry.on_construct<GPULevel>().connect<OnConstruct_GPULevel>();

		registry.on_destroy<MeshCollections>().connect<Destroy_MeshCollections>();
		registry.on_update<GAME::GameManager>().connect<&GAME::GameManager::OnUpdate>();
		registry.on_update<GAME::Player>().connect<&GAME::Player::UpdatePlayer>();
	}

} // namespace DRAW