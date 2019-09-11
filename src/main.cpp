#include <vector>
#include <vulkan/vulkan.hpp>

std::vector<const char *> instanceExtensions = {
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_DISPLAY_EXTENSION_NAME
};

std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

bool validationEnabled = true;

/* global variables (to be put as class members) */
VkInstance instance;
VkPhysicalDevice physicalDevice;

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
	instanceCI.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size());
	instanceCI.ppEnabledLayerNames     = validationLayers.data();
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
 * prints supported instance layers
 */
void printInstanceLayerInfo() {

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
void printInstanceExtensionInfo() {

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
void printDeviceLayerInfo(VkPhysicalDevice device) {

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
void printDeviceExtensionInfo() {

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

/**
 * returns list of supported device features
 */
VkPhysicalDeviceFeatures querySupportedFeatures() {
	VkPhysicalDeviceFeatures supportedFeatures = {};
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return supportedFeatures;
}

VkDevice createLogicalDevice() {

	// initialise queues
	VkDeviceQueueCreateInfo deviceQueueCI = {};
	deviceQueueCI.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCI.queueFamilyIndex = 0;
	deviceQueueCI.queueCount       = 1;
	deviceQueueCI.pQueuePriorities = nullptr;	// default priorities

	// turn on the appropriate (supported) device features
	VkPhysicalDeviceFeatures supportedFeatures = querySupportedFeatures();
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = supportedFeatures.fillModeNonSolid;
	deviceFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;

	VkDeviceCreateInfo deviceCI = {};
	deviceCI.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCI.queueCreateInfoCount    = 1;
	deviceCI.pQueueCreateInfos       = &deviceQueueCI;
	deviceCI.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size());
	deviceCI.ppEnabledLayerNames     = validationLayers.data();
	deviceCI.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
	deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCI.pEnabledFeatures        = &deviceFeatures;

	VkDevice logicalDevice;

	if (vkCreateDevice(physicalDevice, &deviceCI, nullptr, &logicalDevice) != VK_SUCCESS) {
		fputs("Could not create logical device", stderr);
		exit(EXIT_FAILURE);
	}

	return logicalDevice;
}

void cleanup() {

	vkDeviceWaitIdle(logicalDevice);

	vkDestroyDevice(logicalDevice, nullptr);

	vkDestroyInstance(instance, nullptr);

}

int main(int argc, char *argv[]) {

	instance = createInstance("spock");

	std::vector<VkPhysicalDevice> physicalDevices = queryPhysicalDevices();

	physicalDevice = selectPhysicalDevice(physicalDevices);
	printGPUInfo(physicalDevice);

	// printInstanceLayerInfo();
	// printDeviceLayerInfo(physicalDevice);
	// printInstanceExtensionInfo();
	// printDeviceExtensionInfo();

}
