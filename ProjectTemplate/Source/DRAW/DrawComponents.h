#ifndef DRAW_COMPONENTS_H
#define DRAW_COMPONENTS_H


#include "./Utility/load_data_oriented.h"

namespace DRAW
{
	//*** TAGS ***//

	//*** COMPONENTS ***//
	struct VulkanRendererInitialization
	{
		std::string vertexShaderName;
		std::string fragmentShaderName;
		VkClearColorValue clearColor;
		VkClearDepthStencilValue depthStencil;
		float fovDegrees;
		float nearPlane;
		float farPlane;
	};

	struct VulkanRenderer
	{
		GW::GRAPHICS::GVulkanSurface vlkSurface;
		VkDevice device = nullptr;
		VkPhysicalDevice physicalDevice = nullptr;
		VkRenderPass renderPass;
		VkShaderModule vertexShader = nullptr;
		VkShaderModule fragmentShader = nullptr;
		VkPipeline pipeline = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		GW::MATH::GMATRIXF projMatrix;
		VkDescriptorSetLayout descriptorLayout = nullptr;
		VkDescriptorPool descriptorPool = nullptr;
		std::vector<VkDescriptorSet> descriptorSets;
		VkClearValue clrAndDepth[2];
	};

	struct VulkanVertexBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct VulkanIndexBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct GeometryData
	{
		unsigned int indexStart, indexCount, vertexStart;
		inline bool operator < (const GeometryData a) const {
			return indexStart < a.indexStart;
		}
	};
	
	struct GPUInstance
	{
		GW::MATH::GMATRIXF	transform;
		H2B::ATTRIBUTES		matData;
	};

	struct VulkanGPUInstanceBuffer
	{
		unsigned long long element_count = 1;
		std::vector<VkBuffer> buffer;
		std::vector<VkDeviceMemory> memory;
	};

	struct SceneData
	{
		GW::MATH::GVECTORF sunDirection, sunColor, sunAmbient, camPos;
		GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
	};

	struct VulkanUniformBuffer
	{
		std::vector<VkBuffer> buffer;
		std::vector<VkDeviceMemory> memory;
	};

	struct DoNotRender {};

	struct MeshCollections {
		std::vector<entt::entity> meshes;
		GW::MATH::GOBBF collider;
	};

	struct ModelManager {
		std::unordered_map<std::string, MeshCollections> collections;

		MeshCollections& GetCollections(const std::string& name)
		{

			return collections[name];
		}
	};

	struct Camera
	{
		GW::MATH::GMATRIXF camMatrix;
	};

	struct CPULevel
	{
		std::string levelJsonPath;
		std::string h2bFolderPath;
		Level_Data level;

		CPULevel() = default;
		CPULevel(const std::string& jsonPath, const std::string& folderPath) : levelJsonPath(jsonPath), h2bFolderPath(folderPath) {}
	};

	struct GPULevel {};

} // namespace DRAW
#endif // !DRAW_COMPONENTS_H
