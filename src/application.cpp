#include "application.h"


// public:

void Application::run()
{
	initialize();
	mainLoop();
	cleanup();
}

// private:

void Application::mainLoop()
{
	auto window = vc->getWindow();
	auto& framebufferResized = vc->getFrameBool();
	auto& device = vc->getDevice();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}

		auto result = render->drawFrame();

		if (result == DrawFrameResult::SwapchainOutOfDate
			|| result == DrawFrameResult::SwapchainSuboptimal)
		{
			recreateSwapChain();
			continue;
		}
		if (result == DrawFrameResult::FatalError) {
			throw std::runtime_error("render failed");
		}
	}

	vkDeviceWaitIdle(device);
}

void Application::initialize()
{
	vc = new VulkanContext;
	vc->initialize();
	RenderContextDeps rcd = vc->getContextDeps();

	scm = new SwapchainManager(rcd);
	scm->initialize();
	SwapchainDeps scd = scm->getSwapchainDeps();

	rpm = new RenderPassManager(rcd, scd);
	rpm->initialize();
	RenderPassDeps rpd = rpm->getRenderPassDeps();

	cm = new CommandManager(rcd);
	cm->initialize();
	CommandDeps cd = cm->getCommandDeps();

	sym = new SyncManager(rcd);
	sym->initialize();
	SyncDeps syd = sym->getSyncDeps();

	render = new Render(rcd);
}

void Application::cleanup()
{
	render->cleanup();
	delete render;
	sym->cleanup();
	delete sym;
	cm->cleanup();
	delete cm;
	rpm->cleanup();
	delete rpm;
	scm->cleanup();
	delete scm;
	vc->cleanup();
	delete vc;
}

void Application::cleanupSwapChain()
{
	rpm->cleanForRecreate();
	scm->cleanForRecreate();
}
void Application::recreateSwapChain()
{
	//处理窗口最小化（暂停处理）
	vc->recreateWindow();

	//清理旧交换链
	cleanupSwapChain();

	//重新创建调用函数
	scm->createSwapChain();
	scm->createImageViews();
	rpm->createColorResources();
	rpm->createDepthResources();
	rpm->createFramebuffers();
}