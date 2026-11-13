#include "render.h"


// public:

Render::Render(const RenderContextDeps& rcd, const SwapchainDeps& scd, const CommandDeps& cd, const SyncDeps& syd) :
	physicalDevice(rcd.physicalDevice), device(rcd.device), graphicsQueue(rcd.graphicsQueue), presentQueue(rcd.presentQueue),
	swapChain(scd.swapChain), swapChainExtent(scd.swapChainExtent), swapChainImageFormat(scd.swapChainImageFormat), swapChainImages(scd.swapChainImages),
	commandBuffers(cd.commandBuffers), imageAvailableSemaphores(syd.imageAvailableSemaphores), renderFinishedSemaphores(syd.renderFinishedSemaphores), inFlightFences(syd.inFlightFences)
{

}

void Render::initialize()
{
	createStorageImage();
	createCameraUBO();
	createTriangleBuffer();
	createSceneInfoUBO();

	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSets();

	createComputePipeline();
}

DrawFrameResult Render::drawFrame()
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return DrawFrameResult::SwapchainOutOfDate;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		return DrawFrameResult::FatalError;
	}

	vkResetFences(device, 1, &inFlightFences[currentFrame]);
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);


	updateCameraUBO();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}

	transitionImageLayout(commandBuffers[currentFrame], storageImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, VK_ACCESS_SHADER_WRITE_BIT);

	vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, descriptorSets.data(), 0, nullptr);
	vkCmdDispatch(commandBuffers[currentFrame], (swapChainExtent.width + 7) / 8, (swapChainExtent.height + 7) / 8, 1);

	transitionImageLayout(commandBuffers[currentFrame], storageImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

	transitionImageLayout(commandBuffers[currentFrame], swapChainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT);

	VkImageBlit blit{};
	blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = { (int32_t)swapChainExtent.width, (int32_t)swapChainExtent.height, 1 };
	blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = { (int32_t)swapChainExtent.width, (int32_t)swapChainExtent.height, 1 };
	vkCmdBlitImage(commandBuffers[currentFrame], storageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

	transitionImageLayout(commandBuffers[currentFrame], swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0);

	if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to end record command buffer!");
	}


	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit queue!");
	}


	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	DrawFrameResult out = DrawFrameResult::Success;
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		out = DrawFrameResult::SwapchainOutOfDate;
	}
	else if (result == VK_SUBOPTIMAL_KHR) {
		out = DrawFrameResult::SwapchainSuboptimal;
	}
	else if (result != VK_SUCCESS) {
		out = DrawFrameResult::FatalError;
	}


	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return out;
}

void Render::cleanup()
{
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	vkFreeMemory(device, triangleBufferMemory, nullptr);
	vkDestroyBuffer(device, triangleBuffer, nullptr);

	vkFreeMemory(device, sceneInfoUBOMemory, nullptr);
	vkDestroyBuffer(device, sceneInfoUBO, nullptr);

	vkUnmapMemory(device, cameraUBOMemory);
	vkFreeMemory(device, cameraUBOMemory, nullptr);
	vkDestroyBuffer(device, cameraUBO, nullptr);

	vkDestroyImageView(device, storageImageView, nullptr);
	vkDestroyImage(device, storageImage, nullptr);
	vkFreeMemory(device, storageImageMemory, nullptr);
}


// private:

void Render::createComputePipeline()
{
	const fs::path baseDir = GetExeDirectory();
	const fs::path compPath = baseDir/ "assets" / "shaders" / "comp.spv";
	auto computeShaderCode = readFile(compPath);
	VkShaderModule compShaderModule = createShaderModule(computeShaderCode, device);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShaderModule;
	computeShaderStageInfo.pName = "main";
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.stage = computeShaderStageInfo;
	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline!");
	}

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}

void Render::createStorageImage()
{
	createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		storageImage, storageImageMemory, device, physicalDevice);
	storageImageView = createImageView(device, storageImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void Render::createCameraUBO()
{
	VkDeviceSize bufferSize = sizeof(CameraUBO);

	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = bufferSize;
	createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &createInfo, nullptr, &cameraUBO) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create camera ubo buffer!");
	}


	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, cameraUBO, &memReqs);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);
	if (vkAllocateMemory(device, &allocInfo, nullptr, &cameraUBOMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate camera ubo memory!");
	}
	
	if (vkBindBufferMemory(device, cameraUBO, cameraUBOMemory, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to bind camera ubo buffer memory!");
	}

	if (vkMapMemory(device, cameraUBOMemory, 0, sizeof(CameraUBO), 0, &cameraUBOMapped) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to map camera ubo memory!");
	}
}

void Render::createTriangleBuffer()
{
	std::filesystem::path modelPath = GetExeDirectory() / "assets" / "models" / "cube.obj";
	triangles = loadModel(modelPath.string());


	VkDeviceSize bufferSize = sizeof(Triangle) * triangles.size();

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &triangleBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create triangle buffer!");
	}

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, triangleBuffer, &memReqs);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);
	if (vkAllocateMemory(device, &allocInfo, nullptr, &triangleBufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate triangle buffer memory!");
	}
	if (vkBindBufferMemory(device, triangleBuffer, triangleBufferMemory, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to bind triangle buffer memory!");
	}

	void* mapped = nullptr;
	vkMapMemory(device, triangleBufferMemory, 0, bufferSize, 0, &mapped);
	memcpy(mapped, triangles.data(), bufferSize);
	vkUnmapMemory(device, triangleBufferMemory);
}

void Render::createSceneInfoUBO()
{
	SceneInfo sceneInfo;
	sceneInfo.triangleCount = triangles.size();


	VkDeviceSize bufferSize = sizeof(SceneInfo);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &sceneInfoUBO) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create sence info ubo!");
	}


	VkMemoryRequirements memReqs{};
	vkGetBufferMemoryRequirements(device, sceneInfoUBO, &memReqs);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);
	if (vkAllocateMemory(device, &allocInfo, nullptr, &sceneInfoUBOMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate sence info buffer memory!");
	}

	if (vkBindBufferMemory(device, sceneInfoUBO, sceneInfoUBOMemory, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to bind sence info buffer memory!");
	}


	void* mapped = nullptr;
	vkMapMemory(device, sceneInfoUBOMemory, 0, bufferSize, 0, &mapped);
	memcpy(mapped, &sceneInfo, sizeof(sceneInfo));
	vkUnmapMemory(device, sceneInfoUBOMemory);
}

void Render::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding imageBinding{};
	imageBinding.binding = 0;
	imageBinding.descriptorCount = 1;
	imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	imageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding = 1;
	uboBinding.descriptorCount = 1;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding senceInfoUBOBinding{};
	senceInfoUBOBinding.binding = 2;
	senceInfoUBOBinding.descriptorCount = 1;
	senceInfoUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	senceInfoUBOBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding triangleBufferBinding{};
	triangleBufferBinding.binding = 3;
	triangleBufferBinding.descriptorCount = 1;
	triangleBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	triangleBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { imageBinding, uboBinding, senceInfoUBOBinding, triangleBufferBinding };

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Render::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 4> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[0].descriptorCount = 1 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[3].descriptorCount = 1 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Render::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> setLayouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(setLayouts.size());
	allocInfo.pSetLayouts = setLayouts.data();

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo.imageView = storageImageView;

		VkDescriptorBufferInfo uboInfo{};
		uboInfo.buffer = cameraUBO;
		uboInfo.range = sizeof(CameraUBO);

		VkDescriptorBufferInfo sceneInfo{};
		sceneInfo.buffer = sceneInfoUBO;
		sceneInfo.range = sizeof(SceneInfo);

		VkDescriptorBufferInfo triangleInfo{};
		triangleInfo.buffer = triangleBuffer;
		triangleInfo.range = VK_WHOLE_SIZE;
		
		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &uboInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &sceneInfo;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSets[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &triangleInfo;

		vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void Render::updateCameraUBO()
{
	float fovTanHalf = std::tan(glm::radians(60.0f) * 0.5f);
	float aspect = (float)swapChainExtent.width / (float)swapChainExtent.height;
	uint32_t frameIndex = 0;

	CameraUBO ubo{};
	ubo.camPos = glm::vec4(63.7f, 105.0f, 92.6f, 0.0f);
	ubo.camForward = glm::vec4(-0.577f, -0.577f, -0.577f, 0.0f);
	ubo.camRight = glm::vec4(0.707f, 0.0f, -0.707f, 0.0f);
	ubo.camUp = glm::vec4(-0.408f, 0.816f, -0.408f, 0.0f);
	ubo.params = glm::vec4(fovTanHalf, aspect, (float)frameIndex, 0);

	memcpy(cameraUBOMapped, &ubo, sizeof(ubo));
}


// auxiliary functions

std::vector<Triangle> Render::loadModel(const std::string& path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
	{
		throw std::runtime_error("failed to load model!" + warn + err);
	}


	std::vector<Triangle> triangles;

	for (const auto& shape : shapes)
	{
		for (size_t i = 0; i < shape.mesh.indices.size(); i += 3)
		{
			Triangle tri;

			for (int j = 0; j < 3; j++)
			{
				tinyobj::index_t idx = shape.mesh.indices[i + j];
				int vi = idx.vertex_index;

				tri.v[j] = { attrib.vertices[vi * 3 + 0], attrib.vertices[vi * 3 + 1], attrib.vertices[vi * 3 + 2], 0.0f };
			}

			triangles.push_back(tri);
		}
	}

	// debug(
	glm::vec3 minBounds(1e30f);
	glm::vec3 maxBounds(-1e30f);

	for (const auto& tri : triangles) {
		for (int k = 0; k < 3; k++) {
			glm::vec3 v = glm::vec3(tri.v[k]);
			minBounds = glm::min(minBounds, v);
			maxBounds = glm::max(maxBounds, v);
		}
	}

	glm::vec3 center = (minBounds + maxBounds) * 0.5f;
	glm::vec3 extent = maxBounds - minBounds;

	std::cout << "Model Min Bounds: (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ")\n";
	std::cout << "Model Max Bounds: (" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")\n";
	std::cout << "Model Center: (" << center.x << ", " << center.y << ", " << center.z << ")\n";
	std::cout << "Model Size: (" << extent.x << ", " << extent.y << ", " << extent.z << ")\n";
	// )debug

	return triangles;
}