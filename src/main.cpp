#include <cstdio>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

std::vector<const char *> instanceLayers = {
#if defined(DEBUG)
    "VK_LAYER_KHRONOS_validation"
#endif
};

std::vector<const char *> instanceExtensions = {
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_DISPLAY_EXTENSION_NAME
};

std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool validationEnabled = true;

/* global variables (to be put as class members) */
GLFWwindow *window;
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice;
VkDevice logicalDevice;
VkQueue graphicsQueue;

uint32_t graphicsFamilyIndex, presentFamilyIndex, transferFamilyIndex;

void keyboard(GLFWwindow *window, int k, int scancode, int action, int mods) {
	switch (k) {
	case GLFW_KEY_ESCAPE:
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		break;
	}
}

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

void printEnabledInstanceExtensions() {
	for (const char *extension : instanceExtensions) {
		fprintf(stdout, "%s\n", extension);
	}
}

void printEnabledDeviceExtensions() {
	for (const char *extension : deviceExtensions) {
		fprintf(stdout, "%s\n", extension);
	}
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

void initLayers(std::vector<const char *> requestedLayers) {

	if (!requestedInstanceLayersSupported(requestedLayers)) {
		printSupportedInstanceLayers();
		exit(EXIT_FAILURE);
	}

}

// TODO : nicer way of switching between nullws/glfw
void initInstanceExtensions(bool useGLFW) {

	if (useGLFW) {
		instanceExtensions.clear();

		uint32_t glfwExtensionCount;
		const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		for (uint32_t i = 0; i < glfwExtensionCount; i++)
			instanceExtensions.push_back(glfwExtensions[i]);
	}
}

GLFWwindow *createWindow(int width, int height, const char *title) {

	GLFWwindow *window;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(width, height, title, nullptr, nullptr);

	glfwSetKeyCallback(window, keyboard);

	return window;
}

VkInstance createInstance(const char *name) {

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

VkSurfaceKHR createSurface() {

	VkSurfaceKHR surface;

	// TODO : make work with nullws (i.e. no glfw)
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		fputs("Unable to create GLFW surface\n", stderr);
		exit(EXIT_FAILURE);
	}

	return surface;
		
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
		if (queueFamilies[i].queueFlags & queueBits)
			return i;

	}

	return -1;
}

/**
 * get the index of a queue family capable of presenting swapchain images
 */
int getPresentationCapableQueueIndex(VkPhysicalDevice device, VkSurfaceKHR surface) {

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

VkDevice createLogicalDevice() {

	// get graphics queue family index
	graphicsFamilyIndex = getQueueFamilyIndex(physicalDevice, VK_QUEUE_GRAPHICS_BIT);

	// get a queue family
	presentFamilyIndex = getPresentationCapableQueueIndex(physicalDevice, surface);

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
	deviceCI.enabledLayerCount       = static_cast<uint32_t>(instanceLayers.size());
	deviceCI.ppEnabledLayerNames     = instanceLayers.data();
	deviceCI.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
	deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCI.pEnabledFeatures        = &deviceFeatures;

	VkDevice logicalDevice;

	if (vkCreateDevice(physicalDevice, &deviceCI, nullptr, &logicalDevice) != VK_SUCCESS) {
		fputs("Could not create logical device", stderr);
		exit(EXIT_FAILURE);
	}

	vkGetDeviceQueue(logicalDevice, graphicsFamilyIndex, 0, &graphicsQueue);

	return logicalDevice;
}

void cleanup() {

	// vkDeviceWaitIdle(logicalDevice);

	// vkDestroyDevice(logicalDevice, nullptr);

	vkDestroyInstance(instance, nullptr);

}

int main(int argc, char *argv[]) {

	/*
	initLayers(instanceLayers);

	window = createWindow(640, 480, "spock");

	initInstanceExtensions(true);

	instance = createInstance("spock");

	surface = createSurface();

	std::vector<VkPhysicalDevice> physicalDevices = queryPhysicalDevices();
	physicalDevice = selectPhysicalDevice(physicalDevices);

	logicalDevice = createLogicalDevice();

	printEnabledInstanceExtensions();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

	}
	*/

	printf("Display: %s\n", VK_KHR_DISPLAY_EXTENSION_NAME);

	// TODO : tidy up

	// printGPUInfo(physicalDevice);
	// printSupportedInstanceLayers();
	// printSupportedDeviceLayers(physicalDevice);
	// printSupportedInstanceExtensions();
	// printSupportedDeviceExtensions();

}
