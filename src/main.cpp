#include <algorithm>
#include <cstdio>
#include <vector>

#if !defined(USE_NULLWS)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vulkan/vulkan.hpp>

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
	appInfo.sType                 = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName      = name;
	appInfo.applicationVersion    = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName			  = "spock";
	appInfo.engineVersion         = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion            = VK_API_VERSION_1_0;

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
VkPhysicalDeviceFeatures querySupportedFeatures() {
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
	VkPhysicalDeviceFeatures supportedFeatures = querySupportedFeatures();
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

VkSurfaceFormatKHR selectSurfaceFormat(VkPhysicalDevice device, VkSurfaceKHR surface) {

	uint32_t surfaceFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, nullptr);

	std::vector<VkSurfaceFormatKHR> availableFormats(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, availableFormats.data());

	for (VkSurfaceFormatKHR surfaceFormat : availableFormats) {
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return surfaceFormat;
	}

	return availableFormats[0];
}

VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkPhysicalDevice device, VkSurfaceKHR surface) {

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilities);

	return surfaceCapabilities;
}

#if defined(USE_NULLWS)
#else
VkExtent2D getSwapchainExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR surfaceCapabilities) {

	if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() || surfaceCapabilities.currentExtent.height == std::numeric_limits<uint32_t>::min()) {
		
		return surfaceCapabilities.currentExtent;

	}

	if (surfaceCapabilities.currentExtent.width == 0 && surfaceCapabilities.currentExtent.height == 0) {

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
	}

}
#endif

VkSwapchainKHR createSwapchain() {

	VkSurfaceFormatKHR chosenFormat = selectSurfaceFormat(physicalDevice, surface);
	VkSurfaceCapabilitiesKHR surfaceCapabilities = getSurfaceCapabilities(physicalDevice, surface);

	uint32_t minImageCount = std::max(surfaceCapabilities.minImageCount, 3u);

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.surface = surface;
	swapchainCI.minImageCount = minImageCount;
	swapchainCI.imageFormat = chosenFormat.format;
	swapchainCI.imageColorSpace = chosenFormat.colorSpace;
	// swapchainCI.imageExtent = ;
	// swapchainCI.

	VkSwapchainKHR swapchain;

	if (vkCreateSwapchainKHR(logicalDevice, &swapchainCI, nullptr, &swapchain) != VK_SUCCESS) {
		fputs("Could not create swapchain\n", stderr);
		exit(EXIT_FAILURE);
	}

	return swapchain;
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

	// swapchain = createSwapchain();

	loop();

	// TODO : clean up after we're done

	// printGPUInfo(physicalDevice);
	// printSupportedInstanceLayers();
	// printSupportedDeviceLayers(physicalDevice);
	// printSupportedInstanceExtensions();
	// printSupportedDeviceExtensions();

}
