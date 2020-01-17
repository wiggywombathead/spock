#include <algorithm>
#include <cstdio>
#include <fstream>
#include <vector>

#if !defined(USE_NULLWS)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vulkan/vulkan.hpp>

#include "vertex.h"

bool validationEnabled = true;

/* global variables (to be put as class members) */
#if !defined(USE_NULLWS)
GLFWwindow *window;
#endif

VkInstance instance;
VkPhysicalDevice physicalDevice;
VkSurfaceKHR surface;
VkDevice logicalDevice;
VkQueue graphicsQueue, presentQueue;
uint32_t graphicsFamilyIndex, presentFamilyIndex, transferFamilyIndex;

VkSwapchainKHR swapchain;
VkExtent2D swapchainExtent;
VkSurfaceFormatKHR swapchainFormat;
VkPresentModeKHR swapchainPresentMode;
std::vector<VkImageView> swapchainImageViews;
std::vector<VkFramebuffer> swapchainFramebuffers;

VkRenderPass renderpass;

VkDescriptorSetLayout descriptorSetLayout;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;

VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;

VkImage depthBuffer;
VkDeviceMemory depthBufferMemory;
VkImageView depthBufferView;

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

#if !defined(USE_NULLWS)
void keyboard(GLFWwindow *window, int k, int scancode, int action, int mods) {
	switch (k) {
	case GLFW_KEY_ESCAPE:
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		break;
	}
}
#endif

const std::vector<Vertex> vertices = {
    { Vertex({ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}) },
    { Vertex({ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}) },
    { Vertex({-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}) }
};

/**
 * prints supported instance layers
 */
void printSupportedInstanceLayers() {

	uint32_t instanceLayerCount;
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);

	if (instanceLayerCount <= 0) {
		fputs("No instance layers supported", stdout);
		return;
	}

	std::vector<VkLayerProperties> layerProperties(instanceLayerCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, layerProperties.data());

	for (VkLayerProperties properties : layerProperties)
		fprintf(stdout, "%s:\t%s\n", properties.layerName, properties.description);

}

/**
 * prints supported instance extensions
 */
void printSupportedInstanceExtensions() {

	uint32_t instanceExtensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);

	if (instanceExtensionCount <= 0) {
		fputs("No instance extensions supported", stdout);
		return;
	}

	std::vector<VkExtensionProperties> extensionProperties(instanceExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, extensionProperties.data());

	for (VkExtensionProperties properties : extensionProperties)
		fprintf(stdout, "%s\n", properties.extensionName);

}

/**
 * prints supported device layers
 */
void printSupportedDeviceLayers(VkPhysicalDevice device) {

	uint32_t deviceLayerCount;
	vkEnumerateDeviceLayerProperties(device, &deviceLayerCount, nullptr);

	if (deviceLayerCount <= 0) {
		fputs("No device layers supported", stdout);
		return;
	}

	std::vector<VkLayerProperties> layerProperties(deviceLayerCount);
	vkEnumerateDeviceLayerProperties(device, &deviceLayerCount, layerProperties.data());

	for (VkLayerProperties properties : layerProperties)
		fprintf(stdout, "%s:\t%s\n", properties.layerName, properties.description);

}

/**
 * prints supported device extensions
 */
void printSupportedDeviceExtensions() {

	uint32_t deviceExtensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &deviceExtensionCount, nullptr);

	if (deviceExtensionCount <= 0) {
		fputs("No instance extensions supported", stdout);
		return;
	}

	std::vector<VkExtensionProperties> extensionProperties(deviceExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &deviceExtensionCount, extensionProperties.data());

	for (VkExtensionProperties properties : extensionProperties)
		fprintf(stdout, "%s\n", properties.extensionName);

}

bool requestedInstanceLayersSupported(std::vector<const char *> requestedLayers) {

	uint32_t supportedLayerCount;
	vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

	std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
	vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

	for (uint32_t i = 0; i < requestedLayers.size(); i++) {

		bool layerFound = false;
		for (VkLayerProperties layer : supportedLayers) {
			if (strcmp(requestedLayers[i], layer.layerName)) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			fprintf(stderr, "Layer %s not supported\n", requestedLayers[i]);
			return false;
		}
	}

	return true;

}

std::vector<const char *> initLayers() {

	std::vector<const char *> requestedLayers = {
#if defined(DEBUG)
		"VK_LAYER_KHRONOS_validation"
#endif
	};

	if (!requestedInstanceLayersSupported(requestedLayers)) {
		printSupportedInstanceLayers();
		exit(EXIT_FAILURE);
	}

	return requestedLayers;

}

std::vector<const char *> initInstanceExtensions() {

	std::vector<const char *> instanceExtensions;

#if defined(USE_NULLWS)
	instanceExtensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_DISPLAY_EXTENSION_NAME
	};
#else
	uint32_t glfwExtensionCount;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (uint32_t i = 0; i < glfwExtensionCount; i++)
		instanceExtensions.push_back(glfwExtensions[i]);
#endif

	return instanceExtensions;
}

std::vector<const char *> initDeviceExtensions() {

	std::vector<const char *> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	return deviceExtensions;

}

#if !defined(USE_NULLWS)
GLFWwindow *createWindow(int width, int height, const char *title) {

	GLFWwindow *window;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(width, height, title, nullptr, nullptr);

	glfwSetKeyCallback(window, keyboard);

	return window;
}
#endif

VkInstance createInstance(
		const char *name,
		std::vector<const char *> instanceLayers,
		std::vector<const char *> instanceExtensions) {

	VkInstance instance;

	VkApplicationInfo appInfo = {};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = name;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "spock";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceCI = {};
	instanceCI.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCI.pApplicationInfo        = &appInfo;
	instanceCI.enabledLayerCount       = static_cast<uint32_t>(instanceLayers.size());
	instanceCI.ppEnabledLayerNames     = instanceLayers.data();
	instanceCI.enabledExtensionCount   = static_cast<uint32_t>(instanceExtensions.size());
	instanceCI.ppEnabledExtensionNames = instanceExtensions.data();

	if (vkCreateInstance(&instanceCI, nullptr, &instance) != VK_SUCCESS) {
		fputs("Could not create Vulkan instance", stderr);
		exit(EXIT_FAILURE);
	}

	return instance;
}

std::vector<VkPhysicalDevice> queryPhysicalDevices() {

	// must discover connected physical devices before creating logical device
	uint32_t deviceCount;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		fputs("No Vulkan-compatible GPUs found", stderr);
		exit(EXIT_FAILURE);
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

	return physicalDevices;
}

// TODO : other way to select physical device?
VkPhysicalDevice selectPhysicalDevice(std::vector<VkPhysicalDevice> devices) {
	return devices[0];
}

VkSurfaceKHR createSurface() {

	VkSurfaceKHR surface;

#if defined(USE_NULLWS)

	uint32_t propertiesCount;
	vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &propertiesCount, nullptr);

	std::vector<VkDisplayPropertiesKHR> displayProperties(propertiesCount);
	vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &propertiesCount, displayProperties.data());

	// TODO : multi-monitor support would require change here
	VkDisplayKHR nativeDisplay = displayProperties[0].display;

	uint32_t modeCount;
	vkGetDisplayModePropertiesKHR(physicalDevice, nativeDisplay, &modeCount, nullptr);

	std::vector<VkDisplayModePropertiesKHR> displayModeProperties(modeCount);
	vkGetDisplayModePropertiesKHR(physicalDevice, nativeDisplay, &modeCount, displayModeProperties.data());

	VkDisplaySurfaceCreateInfoKHR displaySurfaceCI = {};
	displaySurfaceCI.sType           = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	displaySurfaceCI.displayMode     = displayModeProperties[0].displayMode;
	displaySurfaceCI.planeIndex      = 0;
	displaySurfaceCI.planeStackIndex = 0;
	displaySurfaceCI.transform       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	displaySurfaceCI.globalAlpha     = 0.0f;
	displaySurfaceCI.alphaMode       = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
	displaySurfaceCI.imageExtent     = displayModeProperties[0].parameters.visibleRegion;

	if (vkCreateDisplayPlaneSurfaceKHR(instance, &displaySurfaceCI, nullptr, &surface) != VK_SUCCESS) {
		fputs("Could not create surface\n", stderr);
		exit(EXIT_FAILURE);
	}
#else
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		fputs("Unable to create GLFW surface\n", stderr);
		exit(EXIT_FAILURE);
	}
#endif

	return surface;
		
}

void printGPUInfo(VkPhysicalDevice device) {

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	fprintf(stdout,
			"%s (supports API version %u.%u.%u)\n",
			deviceProperties.deviceName,
			VK_VERSION_MAJOR(deviceProperties.apiVersion),
			VK_VERSION_MINOR(deviceProperties.apiVersion),
			VK_VERSION_PATCH(deviceProperties.apiVersion)
			);

}

/**
 * returns list of supported device features
 */
VkPhysicalDeviceFeatures getSupportedFeatures() {
	VkPhysicalDeviceFeatures supportedFeatures = {};
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return supportedFeatures;
}

/**
 * get the index of a queue family supporting the appropriate flags
 */
int getQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlagBits queueBits) {

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++) {

		// TODO : if we want queues with priorities, need >= 2 queues
		if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & queueBits)
			return i;

	}

	return -1;
}

/**
 * get the index of a queue family capable of presenting swapchain images
 */
int getPresentationCapableQueueFamilyIndex(VkPhysicalDevice device, VkSurfaceKHR surface) {

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++) {

		VkBool32 presentationSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

		if (presentationSupport == VK_TRUE)
			return i;

	}

	return -1;
}

VkDevice createLogicalDevice(std::vector<const char *> deviceExtensions) {

	// get index of graphics queue family
	graphicsFamilyIndex = getQueueFamilyIndex(physicalDevice, VK_QUEUE_GRAPHICS_BIT);

	// get index of presentation-capable queue family
	presentFamilyIndex = getPresentationCapableQueueFamilyIndex(physicalDevice, surface);

	float queuePriorities[] = { 0.0f };

	// initialise queues
	VkDeviceQueueCreateInfo deviceQueueCI = {};
	deviceQueueCI.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCI.queueFamilyIndex = graphicsFamilyIndex;
	deviceQueueCI.queueCount       = 1;
	deviceQueueCI.pQueuePriorities = queuePriorities;	// default priorities

	// turn on the appropriate (supported) device features
	VkPhysicalDeviceFeatures supportedFeatures = getSupportedFeatures();
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = supportedFeatures.fillModeNonSolid;
	deviceFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;

	VkDeviceCreateInfo deviceCI = {};
	deviceCI.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCI.queueCreateInfoCount    = 1;
	deviceCI.pQueueCreateInfos       = &deviceQueueCI;
	deviceCI.enabledLayerCount       = 0;
	deviceCI.ppEnabledLayerNames     = nullptr;
	deviceCI.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
	deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCI.pEnabledFeatures        = &deviceFeatures;

	VkDevice logicalDevice;

	if (vkCreateDevice(physicalDevice, &deviceCI, nullptr, &logicalDevice) != VK_SUCCESS) {
		fputs("Could not create logical device", stderr);
		exit(EXIT_FAILURE);
	}

	vkGetDeviceQueue(logicalDevice, graphicsFamilyIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, presentFamilyIndex, 0, &presentQueue);

	return logicalDevice;
}

std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkPhysicalDevice device, VkSurfaceKHR surface) {

	uint32_t surfaceFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, nullptr);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, surfaceFormats.data());

	return surfaceFormats;
}

VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkPhysicalDevice device, VkSurfaceKHR surface) {

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilities);

	return surfaceCapabilities;
}

std::vector<VkPresentModeKHR> getSurfacePresentModes(VkPhysicalDevice device, VkSurfaceKHR surface) {

	uint32_t presentModesCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, nullptr);

	std::vector<VkPresentModeKHR> presentModes(presentModesCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, presentModes.data());

	return presentModes;
}

VkSurfaceFormatKHR selectSurfaceFormat(VkPhysicalDevice device, VkSurfaceKHR surface, VkFormat desiredFormat) {

	std::vector<VkSurfaceFormatKHR> availableFormats = getSurfaceFormats(device, surface);

	for (VkSurfaceFormatKHR surfaceFormat : availableFormats) {
		if (surfaceFormat.format == desiredFormat && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return surfaceFormat;
	}

	return availableFormats[0];
}

VkPresentModeKHR selectPresentMode(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR desiredPresentMode) {

	std::vector<VkPresentModeKHR> availablePresentModes = getSurfacePresentModes(device, surface);

	for (VkPresentModeKHR presentMode : availablePresentModes) {
		if (presentMode == desiredPresentMode) {
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

#if defined(USE_NULLWS)
VkExtent2D getSwapchainExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities) {

	if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() || surfaceCapabilities.currentExtent.height == std::numeric_limits<uint32_t>::min()) {
		
		VkExtent2D swapchainExtent = {
			.width = static_cast<uint32_t>(0),
			.height = static_cast<uint32_t>(0)
		};

		VkExtent2D extent = swapchainExtent;

		if (swapchainExtent.width < surfaceCapabilities.minImageExtent.width) {
			extent.width = surfaceCapabilities.minImageExtent.width;
		} else if (swapchainExtent.width > surfaceCapabilities.maxImageExtent.width) {
			extent.width = surfaceCapabilities.maxImageExtent.width;
		}

		if (swapchainExtent.height < surfaceCapabilities.minImageExtent.height) {
			extent.height = surfaceCapabilities.minImageExtent.height;
		} else if (swapchainExtent.width > surfaceCapabilities.maxImageExtent.height) {
			extent.height = surfaceCapabilities.maxImageExtent.height;
		}

		return extent;

	}

	/* TODO : deal with this case
	 * This may be the case when the window is minimized, in which case it is impossible to create a
	 * swapchain according to the Valid Usage requirements
	 */
	if (surfaceCapabilities.currentExtent.width == 0 && surfaceCapabilities.currentExtent.height == 0) {
		fputs("I don't know what has happened or what to do :( bad extent\n", stderr);
		return surfaceCapabilities.currentExtent;
	}

	return surfaceCapabilities.currentExtent;
}
#else
VkExtent2D getSwapchainExtentGLFW(GLFWwindow *window, VkSurfaceCapabilitiesKHR surfaceCapabilities) {

	if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() || surfaceCapabilities.currentExtent.height == std::numeric_limits<uint32_t>::min()) {

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		extent.width = std::max(
				surfaceCapabilities.minImageExtent.width,
				std::min(surfaceCapabilities.maxImageExtent.width, extent.width)
				);

		extent.height = std::max(
				surfaceCapabilities.minImageExtent.height,
				std::min(surfaceCapabilities.maxImageExtent.height, extent.height)
				);

		return extent;

	} else {
		return surfaceCapabilities.currentExtent;
	}

}
#endif

bool imageUsageSupported(VkImageUsageFlags desiredUsage, VkSurfaceCapabilitiesKHR surfaceCapabilities) {
	return desiredUsage & surfaceCapabilities.supportedUsageFlags;
}

bool surfaceTransformSupported(VkSurfaceTransformFlagBitsKHR desiredTransform, VkSurfaceCapabilitiesKHR surfaceCapabilities) {
	return surfaceCapabilities.supportedTransforms & desiredTransform;
}

// TODO??
VkSurfaceTransformFlagBitsKHR getSupportedSurfaceTransform(VkSurfaceCapabilitiesKHR surfaceCapabilities) {
}

VkSwapchainKHR createSwapchain() {

	VkSurfaceCapabilitiesKHR surfaceCapabilities = getSurfaceCapabilities(physicalDevice, surface);

	VkSurfaceFormatKHR surfaceFormat = selectSurfaceFormat(physicalDevice, surface, VK_FORMAT_B8G8R8A8_UNORM);
	VkPresentModeKHR surfacePresentMode = selectPresentMode(physicalDevice, surface, VK_PRESENT_MODE_MAILBOX_KHR);

	swapchainFormat = surfaceFormat;
	swapchainPresentMode = surfacePresentMode;

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.surface          = surface;
	swapchainCI.minImageCount    = std::max<uint32_t>(surfaceCapabilities.minImageCount, 3);
	swapchainCI.imageFormat      = swapchainFormat.format;
	swapchainCI.imageColorSpace  = swapchainFormat.colorSpace;

	// store swapchain extent for later
#if defined(USE_NULLWS)
	swapchainExtent = getSwapchainExtent(surfaceCapabilities);
#else
	swapchainExtent = getSwapchainExtentGLFW(window, surfaceCapabilities);
#endif

	swapchainCI.imageExtent = swapchainExtent;

	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (graphicsFamilyIndex == presentFamilyIndex) {
		swapchainCI.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices   = nullptr;
	} else {

		uint32_t queueFamilyIndices[] = { graphicsFamilyIndex, presentFamilyIndex };

		swapchainCI.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		swapchainCI.queueFamilyIndexCount = 2;
		swapchainCI.pQueueFamilyIndices   = queueFamilyIndices;
	}

	swapchainCI.preTransform = surfaceCapabilities.currentTransform;

	// TODO : check for support on this?
	swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCI.presentMode    = surfacePresentMode;
	swapchainCI.clipped        = VK_TRUE;
	swapchainCI.oldSwapchain   = VK_NULL_HANDLE;

	VkSwapchainKHR swapchain;

	if (vkCreateSwapchainKHR(logicalDevice, &swapchainCI, nullptr, &swapchain) != VK_SUCCESS) {
		fputs("Could not create swapchain\n", stderr);
		exit(EXIT_FAILURE);
	}

	return swapchain;
}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask) {

	VkImageViewCreateInfo imageViewCI = {};
	imageViewCI.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCI.image    = image;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format   = format;
	imageViewCI.components = {
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY
	};
	imageViewCI.subresourceRange = {
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	VkImageView imageView;

	vkCreateImageView(logicalDevice, &imageViewCI, nullptr, &imageView);

	return imageView;
}

std::vector<VkImageView> createSwapchainImageViews() {
	
	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainImageCount, nullptr);

	std::vector<VkImage> swapchainImages(swapchainImageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainImageCount, swapchainImages.data());

	std::vector<VkImageView> swapchainImageViews(swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; i++) {

		swapchainImageViews[i] = createImageView(swapchainImages[i], swapchainFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

	}

	return swapchainImageViews;
}

VkFormat selectImageFormat(VkPhysicalDevice device, std::vector<VkFormat> desiredFormats, VkImageTiling tiling, VkFormatFeatureFlags features) {

	for (VkFormat format : desiredFormats) {

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(device, format, &formatProperties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features)
			return format;
	}

	fputs("Unable to find image format supporting the given tiling and features\n", stderr);
	exit(EXIT_FAILURE);
}

VkFormat selectDepthFormat(VkPhysicalDevice device) {

	std::vector<VkFormat> candidateDepthFormats = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};
	
	return selectImageFormat(device, candidateDepthFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

uint32_t getMemoryType(VkPhysicalDevice device, uint32_t typeBits, VkMemoryPropertyFlags desiredProperties) {

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {

		if ((typeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & desiredProperties) == desiredProperties)
			return i;

	}

	fputs("Unable to find an associated memory type for the desired memory properties\n", stderr);
	exit(EXIT_FAILURE);
}

void createImage(
		VkImage &image,
		VkDeviceMemory &imageMemory,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memoryPropertyFlags) {

	VkImageCreateInfo imageCI = {};
	imageCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType     = VK_IMAGE_TYPE_2D;
	imageCI.format        = format;
	imageCI.extent.width  = width;
	imageCI.extent.height = height;
	imageCI.extent.depth  = 1;
	imageCI.mipLevels     = 1;
	imageCI.arrayLayers   = 1;
	imageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling        = tiling;
	imageCI.usage         = usage;
	imageCI.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	// the following two fields are ignored if sharing mode is not _CONCURRENT
	imageCI.queueFamilyIndexCount = 0;
	imageCI.pQueueFamilyIndices   = nullptr;

	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(logicalDevice, &imageCI, nullptr, &image) != VK_SUCCESS) {
		fputs("Failed to create image\n", stderr);
		exit(EXIT_FAILURE);
	}

	// now allocate the memory for the depth buffer image
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAI = {};
	memoryAI.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAI.allocationSize  = memoryRequirements.size;
	memoryAI.memoryTypeIndex = getMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags);

	if (vkAllocateMemory(logicalDevice, &memoryAI, nullptr, &imageMemory) != VK_SUCCESS) {
		fputs("Unable to allocate memory for depth buffer\n", stderr);
		exit(EXIT_FAILURE);
	}

}

VkCommandPool createCommandPool(uint32_t queueFamilyIndex) {

	VkCommandPool commandPool;

	VkCommandPoolCreateInfo commandPoolCI = {};
	commandPoolCI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCI.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCI.queueFamilyIndex = queueFamilyIndex;

	if (vkCreateCommandPool(logicalDevice, &commandPoolCI, nullptr, &commandPool) != VK_SUCCESS) {
		fputs("Unable to create command pool\n", stderr);
		exit(EXIT_FAILURE);
	}

	return commandPool;
}

// TODO: allocate memory for vertex buffer

VkBuffer createVertexBuffer() {

	VkBufferCreateInfo vertexBufferCI = {};
	vertexBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferCI.size = sizeof(vertices[0]) * vertices.size();
	vertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicalDevice, &vertexBufferCI, nullptr, &vertexBuffer) != VK_SUCCESS) {
		fputs("Unable to create vertex buffer\n", stderr);
		exit(EXIT_FAILURE);
	}

	return vertexBuffer;
}

std::vector<VkCommandBuffer> createCommandBuffers(VkCommandPool commandPool) {

	std::vector<VkCommandBuffer> commandBuffers(swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo commandBufferAI = {};
	commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAI.commandPool = commandPool;
	commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAI.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAI, commandBuffers.data()) != VK_SUCCESS) {
		fputs("Failed to allocate command buffers\n", stderr);
		exit(EXIT_FAILURE);
	}

	return commandBuffers;
}

VkDescriptorSetLayout createDescriptorSetLayout() {

	/* Uniform Buffer Object layout */
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
	descriptorSetLayoutBinding.binding            = 0;
	descriptorSetLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBinding.descriptorCount    = 1;
	descriptorSetLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {};
	descriptorSetLayoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCI.bindingCount = 1;
	descriptorSetLayoutCI.pBindings    = &descriptorSetLayoutBinding;

	if (vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		fputs("Failed to create descriptor set layout\n", stderr);
		exit(EXIT_FAILURE);
	}

	return descriptorSetLayout;
}

void createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes = {};

}

void createDescriptorSets() {

}

void recordRenderpasses() {

	for (uint32_t i = 0; i < commandBuffers.size(); i++) {

		VkCommandBufferBeginInfo commandBufferBI = {};
		commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBI.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		if (vkBeginCommandBuffer(commandBuffers[i], &commandBufferBI) != VK_SUCCESS) {
			fputs("Unable to begin command buffer\n", stderr);
		}

		std::array<VkClearValue, 2> clearColors = {};
		clearColors[0].color = { 0.3f, 0.3f, 0.3f, 1.0f };
		clearColors[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderpassBI = {};
		renderpassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpassBI.renderPass = renderpass;
		renderpassBI.framebuffer = swapchainFramebuffers[i];
		renderpassBI.renderArea.offset = { 0, 0 };
		renderpassBI.renderArea.extent = swapchainExtent;
		renderpassBI.clearValueCount = static_cast<uint32_t>(clearColors.size());
		renderpassBI.pClearValues = clearColors.data();

		// start of renderpass
		vkCmdBeginRenderPass(commandBuffers[i], &renderpassBI, VK_SUBPASS_CONTENTS_INLINE);
			
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			// TODO: create descriptor sets
			vkCmdBindDescriptorSets(
					commandBuffers[i],
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayout,
					0,
					1,
					&descriptorSets[i],
					0,
					nullptr
				);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			fputs("Failed to end command buffer\n", stderr);
			exit(EXIT_FAILURE);
		}

	}
	
}

VkCommandBuffer beginCommandRecording(VkCommandPool commandPool) {

	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo commandBufferAI = {};
	commandBufferAI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAI.commandPool        = commandPool;
	commandBufferAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAI.commandBufferCount = 1;

	vkAllocateCommandBuffers(logicalDevice, &commandBufferAI, &commandBuffer);

	VkCommandBufferBeginInfo commandBufferBI = {};
	commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &commandBufferBI);

	return commandBuffer;
}

void submitCommandBuffer(VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue) {

	VkSubmitInfo submitI = {};
	submitI.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitI.commandBufferCount = 1;
	submitI.pCommandBuffers    = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitI, VK_NULL_HANDLE);

	// TODO : change when synchronisation implemented
	vkQueueWaitIdle(queue);

	// TODO : have option to free or just reset command buffer (less $$$)
	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void endCommandRecording(VkCommandBuffer commandBuffer) {

	vkEndCommandBuffer(commandBuffer);

}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {

	VkCommandBuffer commandBuffer = beginCommandRecording(commandPool);

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout           = oldLayout;
	imageMemoryBarrier.newLayout           = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image               = image;
	imageMemoryBarrier.subresourceRange = {
		.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel   = 0,
		.levelCount     = 1,
		.baseArrayLayer = 0,
		.layerCount     = 1
	};

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {

		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

	} else {
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags srcStageFlags, dstStageFlags;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {

		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	} else {

		fprintf(stderr, "Unsupported layout transition: 0x%x to 0x%x\n", oldLayout, newLayout);
		exit(EXIT_FAILURE);
	
	}

	vkCmdPipelineBarrier(
			commandBuffer,
			srcStageFlags,
			dstStageFlags,
			0,				// dependency flags
			0,				// memory barrier count
			nullptr,		// memory barriers
			0,				// buffer memory barrier count
			nullptr,		// buffer memory barriers
			1,				// image memory barrier count
			&imageMemoryBarrier
		);

	endCommandRecording(commandBuffer);

	submitCommandBuffer(commandPool, commandBuffer, graphicsQueue);
}

VkImage createDepthBuffer() {

	VkFormat depthFormat = selectDepthFormat(physicalDevice);

	createImage(
			depthBuffer, depthBufferMemory,
			swapchainExtent.width, swapchainExtent.height, depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);

	vkBindImageMemory(logicalDevice, depthBuffer, depthBufferMemory, 0);

	depthBufferView = createImageView(depthBuffer, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transitionImageLayout(depthBuffer, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	return depthBuffer;
}

VkRenderPass createRenderPass() {

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format         = swapchainFormat.format;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;	// TODO : multisampling
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format         = selectDepthFormat(physicalDevice);
	depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;	// TODO : multisampling
	depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentDescription, 2> attachments = {
		colorAttachment, depthAttachment
	};

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount    = 1;
	subpassDescription.pColorAttachments       = &colorAttachmentReference;
	subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass      = 0;
	subpassDependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask   = 0;
	subpassDependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dependencyFlags = 0;

	VkRenderPassCreateInfo renderpassCI = {};
	renderpassCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderpassCI.pAttachments    = attachments.data();
	renderpassCI.subpassCount    = 1;
	renderpassCI.pSubpasses      = &subpassDescription;
	renderpassCI.dependencyCount = 1;
	renderpassCI.pDependencies   = &subpassDependency;

	VkRenderPass renderpass;

	if (vkCreateRenderPass(logicalDevice, &renderpassCI, nullptr, &renderpass) != VK_SUCCESS) {
		fputs("Unable to create render pass\n", stderr);
		exit(EXIT_FAILURE);
	}

	return renderpass;
}

std::vector<VkFramebuffer> createFramebuffers() {

	std::vector<VkFramebuffer> swapchainFramebuffers(swapchainImageViews.size());

	for (uint32_t i = 0; i < swapchainFramebuffers.size(); i++) {

		std::array<VkImageView, 2> framebufferAttachments = {
			swapchainImageViews[i], depthBufferView
		};

		VkFramebufferCreateInfo framebufferCI = {};
		framebufferCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCI.renderPass      = renderpass;
		framebufferCI.attachmentCount = static_cast<uint32_t>(framebufferAttachments.size());
		framebufferCI.pAttachments    = framebufferAttachments.data();
		framebufferCI.width           = swapchainExtent.width;
		framebufferCI.height          = swapchainExtent.height;
		framebufferCI.layers          = 1;

		VkFramebuffer framebuffer;

		if (vkCreateFramebuffer(logicalDevice, &framebufferCI, nullptr, &framebuffer) != VK_SUCCESS) {
			fputs("Unable to create framebuffer\n", stderr);
			exit(EXIT_FAILURE);
		}

	}

	return swapchainFramebuffers;
}

VkShaderModule createShaderModule(const std::string filename) {

	std::ifstream file(filename);

	if (!file.is_open()) {
		fprintf(stderr, "Could not open file %s\n", filename.c_str());
		return nullptr;
	}

	file.seekg(0, std::ios::end);
	size_t length = file.tellg();

	file.seekg(0);

	std::string source(length, '\0');
	file.read(source.data(), length);

	VkShaderModuleCreateInfo shaderModuleCI = {};
	shaderModuleCI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCI.codeSize = source.length();
	shaderModuleCI.pCode    = reinterpret_cast<const uint32_t *>(source.data());

	VkShaderModule shaderModule;

	if (vkCreateShaderModule(logicalDevice, &shaderModuleCI, nullptr, &shaderModule) != VK_SUCCESS) {
		fprintf(stderr, "Could not create shader module for %s\n", filename.c_str());
		exit(EXIT_FAILURE);
	}

	file.close();

	return shaderModule;
}

// TODO : overloads for different e.g. geometry, etc. shaders?
VkPipeline createGraphicsPipeline(const std::string vertexShaderPath, const std::string fragmentShaderPath) {

	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderPath);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderPath);

	VkPipelineShaderStageCreateInfo vertShaderStageCI = {};
	vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCI.module = vertexShaderModule;
	vertShaderStageCI.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageCI = {};
	fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCI.module = fragmentShaderModule;
	fragShaderStageCI.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertShaderStageCI, fragShaderStageCI
	};

	VkVertexInputBindingDescription vertexInputBindingDescription = Vertex::getInputBindingDescription();
	std::array<VkVertexInputAttributeDescription, Vertex::attributeCount> vertexInputAttributeDescriptions = Vertex::getAttributeDescriptions();
	
	VkPipelineVertexInputStateCreateInfo vertexInputStateCI = {};
	vertexInputStateCI.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCI.vertexBindingDescriptionCount   = 1;
	vertexInputStateCI.pVertexBindingDescriptions      = &vertexInputBindingDescription;
	vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
	vertexInputStateCI.pVertexAttributeDescriptions    = vertexInputAttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = {};
	inputAssemblyStateCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCI.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = swapchainExtent.width;
	viewport.height   = swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportStateCI = {};
	viewportStateCI.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCI.viewportCount = 1;
	viewportStateCI.pViewports    = &viewport;
	viewportStateCI.scissorCount  = 1;
	viewportStateCI.pScissors     = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCI = {};
	rasterizationStateCI.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCI.depthClampEnable        = VK_FALSE;
	rasterizationStateCI.rasterizerDiscardEnable = VK_TRUE;
	rasterizationStateCI.polygonMode             = VK_POLYGON_MODE_FILL;
	rasterizationStateCI.cullMode                = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCI.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCI.depthBiasEnable         = VK_FALSE;
	rasterizationStateCI.lineWidth               = 1.0f;

	// TODO : detect multisampling support and implement
	VkPipelineMultisampleStateCreateInfo multisampleStateCI = {};
	multisampleStateCI.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCI.sampleShadingEnable  = VK_FALSE;
	multisampleStateCI.minSampleShading     = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = {};
	depthStencilStateCI.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable       = VK_TRUE;
	depthStencilStateCI.depthWriteEnable      = VK_TRUE;
	depthStencilStateCI.depthCompareOp        = VK_COMPARE_OP_LESS;
	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCI.stencilTestEnable     = VK_FALSE;
	depthStencilStateCI.minDepthBounds        = 0.0f;
	depthStencilStateCI.maxDepthBounds        = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCI = {};
	colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCI.logicOpEnable = VK_FALSE;
	colorBlendStateCI.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCI.attachmentCount = 1;
	colorBlendStateCI.pAttachments = &colorBlendAttachment;
	colorBlendStateCI.blendConstants[0] = 0.0f;
	colorBlendStateCI.blendConstants[1] = 0.0f;
	colorBlendStateCI.blendConstants[2] = 0.0f;
	colorBlendStateCI.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts    = &descriptorSetLayout;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCI, nullptr, &pipelineLayout) != VK_SUCCESS) {
		fputs("Could not create pipeline layout\n", stderr);
		exit(EXIT_FAILURE);
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineCI = {};
	graphicsPipelineCI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCI.stageCount          = 2;
	graphicsPipelineCI.pStages             = shaderStages;
	graphicsPipelineCI.pVertexInputState   = &vertexInputStateCI;
	graphicsPipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
	graphicsPipelineCI.pViewportState      = &viewportStateCI;
	graphicsPipelineCI.pRasterizationState = &rasterizationStateCI;
	graphicsPipelineCI.pMultisampleState   = &multisampleStateCI;
	graphicsPipelineCI.pDepthStencilState  = &depthStencilStateCI;
	graphicsPipelineCI.pColorBlendState    = &colorBlendStateCI;
	graphicsPipelineCI.layout              = pipelineLayout;
	graphicsPipelineCI.renderPass          = renderpass;

	VkPipeline graphicsPipeline;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCI, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		fputs("Could not create graphics pipeline\n", stderr);
		exit(EXIT_FAILURE);
	}

	return graphicsPipeline;
}

void loop() {
#if defined(USE_NULLWS)
	while (1)
		;
#else
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

	}
#endif
}

void cleanup() {

	vkDeviceWaitIdle(logicalDevice);

	vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);

	// vkDestroyShaderModule(logicalDevice, 

	for (VkImageView imageView : swapchainImageViews) {
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}

	vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);

	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

	vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyDevice(logicalDevice, nullptr);

	vkDestroyInstance(instance, nullptr);

}

int main(int argc, char *argv[]) {

#if !defined(USE_NULLWS)
	window = createWindow(640, 480, "spock");
#endif

	std::vector<const char *> instanceLayers = initLayers();
	std::vector<const char *> instanceExtensions = initInstanceExtensions();

	instance = createInstance("spock", instanceLayers, instanceExtensions);
	// printEnabledInstanceExtensions();

	std::vector<const char *> deviceExtensions = initDeviceExtensions();

	std::vector<VkPhysicalDevice> physicalDevices = queryPhysicalDevices();
	physicalDevice = selectPhysicalDevice(physicalDevices);

	surface = createSurface();

	logicalDevice = createLogicalDevice(deviceExtensions);

	swapchain = createSwapchain();
	swapchainImageViews = createSwapchainImageViews();

	renderpass = createRenderPass();

	descriptorSetLayout = createDescriptorSetLayout();

	graphicsPipeline = createGraphicsPipeline("spirv/test.vert", "spirv/test.frag");

	swapchainFramebuffers = createFramebuffers();

	commandPool = createCommandPool(graphicsFamilyIndex);

	vertexBuffer = createVertexBuffer();
	depthBuffer = createDepthBuffer();

	commandBuffers = createCommandBuffers(commandPool);

	loop();

	// TODO : clean up after we're done

	// printGPUInfo(physicalDevice);
	// printSupportedInstanceLayers();
	// printSupportedDeviceLayers(physicalDevice);
	// printSupportedInstanceExtensions();
	// printSupportedDeviceExtensions();

}
