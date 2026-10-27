#pragma once

#include "utils.h"


class Render
{
public:
	Render(const RenderContextDeps& rcd);
	void initialize();
	void cleanup();
	DrawFrameResult drawFrame();

private:
	void createComputePipeline();

private:
	const VkDevice& device;

private:
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
};