#pragma once

#include "utils.h"
#include "vulkanContext.h"
#include "swapchainManager.h"
#include "renderPassManager.h"
#include "commandManager.h"
#include "syncManager.h"
#include "render/render.h"

#include <iostream>

class Application
{
public:
	void run();

private:
	void initialize();
	void mainLoop();
	void cleanup();

	void cleanupSwapChain();
	void recreateSwapChain();

private:
	VulkanContext* vc = nullptr;
	SwapchainManager* scm = nullptr;
	RenderPassManager* rpm = nullptr;
	CommandManager* cm = nullptr;
	SyncManager* sym = nullptr;
	Render* render = nullptr;
};
