#pragma once

#include "utils.h"


class Render
{
public:
	Render(const RenderContextDeps& rcd, const SwapchainDeps& scd, const CommandDeps& cd, const SyncDeps& syd);
	void initialize();
	void cleanup();
	DrawFrameResult drawFrame();

private:
	void createStorageImage();

	void createCameraUBO();

	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();

	void createComputePipeline();

private:
	void updateCameraUBO();


private:
	const VkPhysicalDevice& physicalDevice;
	const VkDevice& device;
	const VkQueue& graphicsQueue;
	const VkQueue& presentQueue;

	const VkSwapchainKHR& swapChain;
	const VkExtent2D& swapChainExtent;
	const VkFormat& swapChainImageFormat;
	const std::vector<VkImage>& swapChainImages;

	const std::vector<VkCommandBuffer>& commandBuffers;

	const std::vector<VkSemaphore>& imageAvailableSemaphores;
	const std::vector<VkSemaphore>& renderFinishedSemaphores;
	const std::vector<VkFence>& inFlightFences;

private:
	uint32_t currentFrame = 0;

	void* cameraUBOMapped = nullptr;

	VkImage storageImage;
	VkDeviceMemory storageImageMemory;
	VkImageView storageImageView;

	VkBuffer cameraUBO;
	VkDeviceMemory cameraUBOMemory;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
};


struct CameraUBO
{
	glm::vec4 camPos;
	glm::vec4 camForward;
	glm::vec4 camRight;
	glm::vec4 camUp;
	glm::vec4 params;
};