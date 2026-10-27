#include "render.h"


// public:

Render::Render(const RenderContextDeps& rcd):
	device(rcd.device)
{

}

void Render::initialize()
{
	createComputePipeline();
}

DrawFrameResult Render::drawFrame()
{
	std::cout << "here is drawFrame" << std::endl;

	return DrawFrameResult::Success;
}

void Render::cleanup()
{
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

// private:

void Render::createComputePipeline()
{
	const fs::path baseDir = GetExeDirectory();
	const fs::path compPath = baseDir / "shaders" / "comp.spv";
	auto computeShaderCode = readFile(compPath);
	VkShaderModule compShaderModule = createShaderModule(computeShaderCode, device);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShaderModule;
	computeShaderStageInfo.pName = "main";
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
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
}