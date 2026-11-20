#include "osv_vulkan.h"
#include "osv_stitch_spv.h"
#include "direct_reframe_spv.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <algorithm>

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                               uint32_t typeFilter,
                               VkMemoryPropertyFlags properties,
                               bool allowFallback = false,
                               VkMemoryPropertyFlags fallbackProperties = 0)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    if (allowFallback) {
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i))
                && (memProperties.memoryTypes[i].propertyFlags & fallbackProperties)
                       == fallbackProperties) {
                return i;
            }
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

OsvVulkanStitcher::OsvVulkanStitcher() {}

OsvVulkanStitcher::~OsvVulkanStitcher()
{
    cleanup();
}

bool OsvVulkanStitcher::initialize(const std::string &spv_shader_path)
{
    if (device_ == VK_NULL_HANDLE && !initVulkanCore()) {
        std::cerr << "[OSV-Vulkan] Failed to initialize Vulkan core." << std::endl;
        return false;
    }
    if (!createPipeline(spv_shader_path)) {
        std::cerr << "[OSV-Vulkan] Failed to create compute pipeline." << std::endl;
        return false;
    }
    return true;
}

bool OsvVulkanStitcher::initializeReframe(const std::string &reframe_spv_path)
{
    if (device_ == VK_NULL_HANDLE && !initVulkanCore()) {
        std::cerr << "[OSV-Vulkan] Failed to initialize Vulkan core." << std::endl;
        return false;
    }
    if (!createReframePipeline(reframe_spv_path)) {
        std::cerr << "[OSV-Vulkan] Failed to create direct reframe pipeline." << std::endl;
        return false;
    }
    return true;
}

void OsvVulkanStitcher::cleanup()
{
    freeResources();

    if (device_ != VK_NULL_HANDLE) {
        if (command_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, command_pool_, nullptr);
            command_pool_ = VK_NULL_HANDLE;
        }
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }
        if (pipeline_layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
            pipeline_layout_ = VK_NULL_HANDLE;
        }
        if (desc_layout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, desc_layout_, nullptr);
            desc_layout_ = VK_NULL_HANDLE;
        }
        if (shader_module_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, shader_module_, nullptr);
            shader_module_ = VK_NULL_HANDLE;
        }
        if (reframe_pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, reframe_pipeline_, nullptr);
            reframe_pipeline_ = VK_NULL_HANDLE;
        }
        if (reframe_pipeline_layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, reframe_pipeline_layout_, nullptr);
            reframe_pipeline_layout_ = VK_NULL_HANDLE;
        }
        if (reframe_desc_layout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, reframe_desc_layout_, nullptr);
            reframe_desc_layout_ = VK_NULL_HANDLE;
        }
        if (reframe_shader_module_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, reframe_shader_module_, nullptr);
            reframe_shader_module_ = VK_NULL_HANDLE;
        }
        if (reframe_ubo_mem_ != VK_NULL_HANDLE) {
            if (mapped_reframe_ubo_) {
                vkUnmapMemory(device_, reframe_ubo_mem_);
                mapped_reframe_ubo_ = nullptr;
            }
            vkFreeMemory(device_, reframe_ubo_mem_, nullptr);
            reframe_ubo_mem_ = VK_NULL_HANDLE;
        }
        if (reframe_ubo_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, reframe_ubo_, nullptr);
            reframe_ubo_ = VK_NULL_HANDLE;
        }
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

bool OsvVulkanStitcher::initVulkanCore()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "OSV Native Vulkan Stitcher";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "OSV-Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
        return false;
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0)
        return false;

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    // Prefer dedicated or integrated GPU supporting compute
    for (const auto &dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                physical_device_ = dev;
                queue_family_index_ = i;
                std::cout << "[OSV-Vulkan] Using GPU: " << props.deviceName << std::endl;
                break;
            }
        }
        if (physical_device_ != VK_NULL_HANDLE)
            break;
    }

    if (physical_device_ == VK_NULL_HANDLE)
        return false;

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queue_family_index_;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

    if (vkCreateDevice(physical_device_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(device_, queue_family_index_, 0, &compute_queue_);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queue_family_index_;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &command_pool_) != VK_SUCCESS) {
        return false;
    }

    return true;
}

bool OsvVulkanStitcher::createPipeline(const std::string &spv_path)
{
    std::vector<char> buffer;
    const uint32_t *code = nullptr;
    size_t code_size = 0;

    if (!spv_path.empty()) {
        std::ifstream file(spv_path, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            size_t fileSize = (size_t) file.tellg();
            buffer.resize(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
            code = reinterpret_cast<const uint32_t *>(buffer.data());
            code_size = buffer.size();
        } else {
            std::cerr << "[OSV-Vulkan] Shader file not found: " << spv_path
                      << ", falling back to embedded SPIR-V shader." << std::endl;
        }
    }

    if (!code) {
        code = osv_stitch_spv;
        code_size = osv_stitch_spv_size;
    }

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = code_size;
    moduleCreateInfo.pCode = code;

    if (vkCreateShaderModule(device_, &moduleCreateInfo, nullptr, &shader_module_) != VK_SUCCESS) {
        return false;
    }

    // Descriptor Layout:
    // Binding 0: storage image (output equirectangular)
    // Binding 1: combined image sampler (master lens)
    // Binding 2: combined image sampler (slave lens)
    // Binding 3: combined image sampler (128x1 DP seam LUT)
    VkDescriptorSetLayoutBinding bindings[4]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 4;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &desc_layout_) != VK_SUCCESS) {
        return false;
    }

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &desc_layout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipeline_layout_)
        != VK_SUCCESS) {
        return false;
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = shader_module_;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = pipeline_layout_;

    if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_)
        != VK_SUCCESS) {
        return false;
    }

    return true;
}

bool OsvVulkanStitcher::createReframePipeline(const std::string &spv_path)
{
    std::vector<char> buffer;
    const uint32_t *code = nullptr;
    size_t code_size = 0;

    if (!spv_path.empty()) {
        std::ifstream file(spv_path, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            size_t fileSize = (size_t) file.tellg();
            buffer.resize(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
            code = reinterpret_cast<const uint32_t *>(buffer.data());
            code_size = buffer.size();
        } else {
            std::cerr << "[OSV-Vulkan] Reframe shader file not found: " << spv_path
                      << ", falling back to embedded SPIR-V shader." << std::endl;
        }
    }

    if (!code) {
        code = direct_reframe_spv;
        code_size = direct_reframe_spv_size;
    }

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = code_size;
    moduleCreateInfo.pCode = code;

    if (vkCreateShaderModule(device_, &moduleCreateInfo, nullptr, &reframe_shader_module_)
        != VK_SUCCESS) {
        return false;
    }

    VkDescriptorSetLayoutBinding bindings[5]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 5;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &reframe_desc_layout_)
        != VK_SUCCESS) {
        return false;
    }

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &reframe_desc_layout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &reframe_pipeline_layout_)
        != VK_SUCCESS) {
        return false;
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = reframe_shader_module_;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = reframe_pipeline_layout_;

    if (vkCreateComputePipelines(device_,
                                 VK_NULL_HANDLE,
                                 1,
                                 &pipelineInfo,
                                 nullptr,
                                 &reframe_pipeline_)
        != VK_SUCCESS) {
        return false;
    }

    // Allocate persistent host-visible UBO for ReframeUniforms
    VkBufferCreateInfo bInfo{};
    bInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bInfo.size = sizeof(ReframeUniforms);
    bInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &bInfo, nullptr, &reframe_ubo_) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device_, reframe_ubo_, &req);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = req.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        physical_device_,
        req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device_, &allocInfo, nullptr, &reframe_ubo_mem_) != VK_SUCCESS) {
        return false;
    }
    vkBindBufferMemory(device_, reframe_ubo_, reframe_ubo_mem_, 0);
    vkMapMemory(device_, reframe_ubo_mem_, 0, sizeof(ReframeUniforms), 0, &mapped_reframe_ubo_);

    return true;
}

void OsvVulkanStitcher::freeResources()
{
    if (device_ == VK_NULL_HANDLE)
        return;

    if (fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, fence_, nullptr);
        fence_ = VK_NULL_HANDLE;
    }
    if (cmd_ != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(device_, command_pool_, 1, &cmd_);
        cmd_ = VK_NULL_HANDLE;
    }
    if (descPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descPool_, nullptr);
        descPool_ = VK_NULL_HANDLE;
        descSet_ = VK_NULL_HANDLE;
    }
    if (reframe_desc_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, reframe_desc_pool_, nullptr);
        reframe_desc_pool_ = VK_NULL_HANDLE;
        reframe_desc_set_ = VK_NULL_HANDLE;
    }
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    if (samplerSeam_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_, samplerSeam_, nullptr);
        samplerSeam_ = VK_NULL_HANDLE;
    }
    if (viewMaster_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, viewMaster_, nullptr);
        viewMaster_ = VK_NULL_HANDLE;
    }
    if (viewSlave_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, viewSlave_, nullptr);
        viewSlave_ = VK_NULL_HANDLE;
    }
    if (viewOut_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, viewOut_, nullptr);
        viewOut_ = VK_NULL_HANDLE;
    }
    if (viewSeamLut_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, viewSeamLut_, nullptr);
        viewSeamLut_ = VK_NULL_HANDLE;
    }
    if (imgMaster_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_, imgMaster_, nullptr);
        imgMaster_ = VK_NULL_HANDLE;
    }
    if (imgSlave_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_, imgSlave_, nullptr);
        imgSlave_ = VK_NULL_HANDLE;
    }
    if (imgOut_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_, imgOut_, nullptr);
        imgOut_ = VK_NULL_HANDLE;
    }
    if (imgSeamLut_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_, imgSeamLut_, nullptr);
        imgSeamLut_ = VK_NULL_HANDLE;
    }
    if (memMaster_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memMaster_, nullptr);
        memMaster_ = VK_NULL_HANDLE;
    }
    if (memSlave_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memSlave_, nullptr);
        memSlave_ = VK_NULL_HANDLE;
    }
    if (memOut_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memOut_, nullptr);
        memOut_ = VK_NULL_HANDLE;
    }
    if (memSeamLut_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memSeamLut_, nullptr);
        memSeamLut_ = VK_NULL_HANDLE;
    }
    if (stageInMasterMem_ != VK_NULL_HANDLE) {
        if (mappedInMaster_) {
            vkUnmapMemory(device_, stageInMasterMem_);
            mappedInMaster_ = nullptr;
        }
        vkFreeMemory(device_, stageInMasterMem_, nullptr);
        stageInMasterMem_ = VK_NULL_HANDLE;
    }
    if (stageInSlaveMem_ != VK_NULL_HANDLE) {
        if (mappedInSlave_) {
            vkUnmapMemory(device_, stageInSlaveMem_);
            mappedInSlave_ = nullptr;
        }
        vkFreeMemory(device_, stageInSlaveMem_, nullptr);
        stageInSlaveMem_ = VK_NULL_HANDLE;
    }
    if (stageOutMem_ != VK_NULL_HANDLE) {
        if (mappedOut_) {
            vkUnmapMemory(device_, stageOutMem_);
            mappedOut_ = nullptr;
        }
        vkFreeMemory(device_, stageOutMem_, nullptr);
        stageOutMem_ = VK_NULL_HANDLE;
    }
    if (stageSeamLutMem_ != VK_NULL_HANDLE) {
        if (mappedSeamLut_) {
            vkUnmapMemory(device_, stageSeamLutMem_);
            mappedSeamLut_ = nullptr;
        }
        vkFreeMemory(device_, stageSeamLutMem_, nullptr);
        stageSeamLutMem_ = VK_NULL_HANDLE;
    }
    if (stageInMaster_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, stageInMaster_, nullptr);
        stageInMaster_ = VK_NULL_HANDLE;
    }
    if (stageInSlave_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, stageInSlave_, nullptr);
        stageInSlave_ = VK_NULL_HANDLE;
    }
    if (stageOut_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, stageOut_, nullptr);
        stageOut_ = VK_NULL_HANDLE;
    }
    if (stageSeamLut_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, stageSeamLut_, nullptr);
        stageSeamLut_ = VK_NULL_HANDLE;
    }

    alloc_sensor_w_ = 0;
    alloc_sensor_h_ = 0;
    alloc_out_w_ = 0;
    alloc_out_h_ = 0;
}

bool OsvVulkanStitcher::ensureResources(uint32_t sensor_w,
                                        uint32_t sensor_h,
                                        uint32_t out_w,
                                        uint32_t out_h)
{
    if (alloc_sensor_w_ == sensor_w && alloc_sensor_h_ == sensor_h && alloc_out_w_ == out_w
        && alloc_out_h_ == out_h && cmd_ != VK_NULL_HANDLE) {
        return true;
    }

    freeResources();

    alloc_sensor_w_ = sensor_w;
    alloc_sensor_h_ = sensor_h;
    alloc_out_w_ = out_w;
    alloc_out_h_ = out_h;

    size_t inBytes = sensor_w * sensor_h * 4;
    size_t outBytes = out_w * out_h * 4;

    auto createBuf = [&](size_t size,
                         VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags props,
                         VkBuffer &buf,
                         VkDeviceMemory &mem,
                         bool allowFallback = false,
                         VkMemoryPropertyFlags fallbackProps = 0) {
        VkBufferCreateInfo bInfo{};
        bInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bInfo.size = size;
        bInfo.usage = usage;
        bInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device_, &bInfo, nullptr, &buf);

        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device_, buf, &req);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = req.size;
        allocInfo.memoryTypeIndex = findMemoryType(physical_device_,
                                                   req.memoryTypeBits,
                                                   props,
                                                   allowFallback,
                                                   fallbackProps);
        vkAllocateMemory(device_, &allocInfo, nullptr, &mem);
        vkBindBufferMemory(device_, buf, mem, 0);
    };

    createBuf(inBytes,
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              stageInMaster_,
              stageInMasterMem_);
    createBuf(inBytes,
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              stageInSlave_,
              stageInSlaveMem_);
    // Prefer HOST_CACHED memory for GPU->CPU readback to avoid 45ms uncached write-combining read stalls on Intel/AMD GPUs
    createBuf(outBytes,
              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
              stageOut_,
              stageOutMem_,
              true,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    createBuf(128 * 4 * sizeof(float),
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              stageSeamLut_,
              stageSeamLutMem_);

    vkMapMemory(device_, stageInMasterMem_, 0, inBytes, 0, &mappedInMaster_);
    vkMapMemory(device_, stageInSlaveMem_, 0, inBytes, 0, &mappedInSlave_);
    vkMapMemory(device_, stageOutMem_, 0, outBytes, 0, &mappedOut_);
    vkMapMemory(device_, stageSeamLutMem_, 0, 128 * 4 * sizeof(float), 0, &mappedSeamLut_);

    auto createImg =
        [&](uint32_t w, uint32_t h, VkImageUsageFlags usage, VkImage &img, VkDeviceMemory &mem, VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM) {
            VkImageCreateInfo iInfo{};
            iInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            iInfo.imageType = VK_IMAGE_TYPE_2D;
            iInfo.format = fmt;
            iInfo.extent = {w, h, 1};
            iInfo.mipLevels = 1;
            iInfo.arrayLayers = 1;
            iInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            iInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            iInfo.usage = usage;
            iInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            iInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            vkCreateImage(device_, &iInfo, nullptr, &img);

            VkMemoryRequirements req;
            vkGetImageMemoryRequirements(device_, img, &req);
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = req.size;
            allocInfo.memoryTypeIndex = findMemoryType(physical_device_,
                                                       req.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(device_, &allocInfo, nullptr, &mem);
            vkBindImageMemory(device_, img, mem, 0);
        };

    createImg(sensor_w,
              sensor_h,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              imgMaster_,
              memMaster_);
    createImg(sensor_w,
              sensor_h,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              imgSlave_,
              memSlave_);
    createImg(out_w,
              out_h,
              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
              imgOut_,
              memOut_);
    createImg(128,
              1,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              imgSeamLut_,
              memSeamLut_,
              VK_FORMAT_R32G32B32A32_SFLOAT);

    auto createView = [&](VkImage img, VkImageView &view, VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM) {
        VkImageViewCreateInfo vInfo{};
        vInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vInfo.image = img;
        vInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vInfo.format = fmt;
        vInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vInfo.subresourceRange.levelCount = 1;
        vInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device_, &vInfo, nullptr, &view);
    };

    createView(imgMaster_, viewMaster_);
    createView(imgSlave_, viewSlave_);
    createView(imgOut_, viewOut_);
    createView(imgSeamLut_, viewSeamLut_, VK_FORMAT_R32G32B32A32_SFLOAT);

    VkSamplerCreateInfo sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sInfo.magFilter = VK_FILTER_LINEAR;
    sInfo.minFilter = VK_FILTER_LINEAR;
    sInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(device_, &sInfo, nullptr, &sampler_);

    VkSamplerCreateInfo sSeamInfo{};
    sSeamInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sSeamInfo.magFilter = VK_FILTER_LINEAR;
    sSeamInfo.minFilter = VK_FILTER_LINEAR;
    sSeamInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sSeamInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sSeamInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(device_, &sSeamInfo, nullptr, &samplerSeam_);

    if (desc_layout_ != VK_NULL_HANDLE) {
        VkDescriptorPoolSize poolSizes[2]{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[0].descriptorCount = 1;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 3;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;
        vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descPool_);

        VkDescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocInfo.descriptorPool = descPool_;
        setAllocInfo.descriptorSetCount = 1;
        setAllocInfo.pSetLayouts = &desc_layout_;
        vkAllocateDescriptorSets(device_, &setAllocInfo, &descSet_);

        VkDescriptorImageInfo outInfo{};
        outInfo.imageView = viewOut_;
        outInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkDescriptorImageInfo masterInfo{};
        masterInfo.sampler = sampler_;
        masterInfo.imageView = viewMaster_;
        masterInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo slaveInfo{};
        slaveInfo.sampler = sampler_;
        slaveInfo.imageView = viewSlave_;
        slaveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo seamInfo{};
        seamInfo.sampler = samplerSeam_;
        seamInfo.imageView = viewSeamLut_;
        seamInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writes[4]{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descSet_;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[0].pImageInfo = &outInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descSet_;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &masterInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = descSet_;
        writes[2].dstBinding = 2;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[2].pImageInfo = &slaveInfo;

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].dstSet = descSet_;
        writes[3].dstBinding = 3;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[3].pImageInfo = &seamInfo;

        vkUpdateDescriptorSets(device_, 4, writes, 0, nullptr);
    }

    if (reframe_desc_layout_ != VK_NULL_HANDLE) {
        updateReframeDescriptors();
    }

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = command_pool_;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device_, &cmdAllocInfo, &cmd_);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(device_, &fenceInfo, nullptr, &fence_);

    return true;
}

namespace {

struct Vec3f {
    float x, y, z;
    Vec3f operator+(const Vec3f& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3f operator-(const Vec3f& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3f operator*(float s) const { return {x * s, y * s, z * s}; }
};

static inline Vec3f crossVec(const Vec3f& a, const Vec3f& b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

static inline float dotVec(const Vec3f& a, const Vec3f& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3f normVec(const Vec3f& v) {
    float l = std::sqrt(dotVec(v, v));
    return l > 1e-6f ? Vec3f{v.x / l, v.y / l, v.z / l} : v;
}

static inline Vec3f quatRotate(const float q[4], const Vec3f& v) {
    Vec3f qvec = {q[1], q[2], q[3]};
    Vec3f uv = crossVec(qvec, v);
    Vec3f uuv = crossVec(qvec, uv);
    return v + (uv * q[0] + uuv) * 2.0f;
}

static inline uint16_t floatToHalf(float f) {
    uint32_t x = 0;
    std::memcpy(&x, &f, sizeof(x));
    uint32_t sign = (x >> 16) & 0x8000u;
    int32_t exp = static_cast<int32_t>((x >> 23) & 0xFFu) - 127 + 15;
    uint32_t mant = x & 0x007FFFFFu;
    if (exp <= 0) {
        if (exp < -10) return static_cast<uint16_t>(sign);
        mant |= 0x00800000u;
        uint32_t shift = static_cast<uint32_t>(14 - exp);
        return static_cast<uint16_t>(sign | ((mant + (1u << (shift - 1))) >> shift));
    }
    if (exp >= 31) {
        return static_cast<uint16_t>(sign | 0x7C00u);
    }
    return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exp) << 10) | ((mant + 0x00001000u) >> 13));
}

static inline float kannalaBrandtRd(float th, const LensCalibration& cal) {
    // 5-term Kannala-Brandt model (k1..k5) is strictly monotonic past 105 deg.
    // Only clamp to the 4-term stationary peak if k5 is absent (legacy fallback).
    float safeTheta = th;
    if (std::fabs(cal.k5) <= 1e-8f) {
        float thPeak = (cal.k4 < -0.002f) ? 1.63587f : 1.70008f;
        safeTheta = std::min(th, thPeak);
    }
    float t2 = safeTheta * safeTheta;
    float t4 = t2 * t2;
    float t6 = t4 * t2;
    float t8 = t4 * t4;
    float t10 = t8 * t2;
    return safeTheta * (1.0f + cal.k1 * t2 + cal.k2 * t4 + cal.k3 * t6 + cal.k4 * t8 + cal.k5 * t10);
}

static inline bool projectLens(const Vec3f& dLens, const LensCalibration& cal, uint32_t w, uint32_t h, float& px, float& py) {
    float rho = std::sqrt(dLens.x * dLens.x + dLens.y * dLens.y);
    float th = std::atan2(rho, dLens.z);
    // With 5-term k5, both lenses remain strictly monotonic up to the 99 deg optical rim.
    float thLimit = (std::fabs(cal.k5) > 1e-8f)
                        ? 1.7279f // 99.0 deg
                        : ((cal.k4 < -0.002f) ? 1.6057f : 1.6650f);
    if (th > thLimit) return false;
    float rd = kannalaBrandtRd(th, cal);
    float safe_rho = std::max(rho, 1e-6f);
    float sx = (float)w / 3840.0f;
    float sy = (float)h / 3840.0f;
    px = (cal.cx + cal.fx * rd * (dLens.x / safe_rho)) * sx;
    py = (cal.cy + cal.fy * rd * (dLens.y / safe_rho)) * sy;
    return (px >= 0.0f && px < (float)(w - 1) && py >= 0.0f && py < (float)(h - 1));
}

static inline float sampleLuma(const uint8_t* rgba, size_t stride, float px, float py) {
    int x0 = (int)px;
    int y0 = (int)py;
    const uint8_t* p = rgba + y0 * stride + x0 * 4;
    return (p[0] * 0.299f + p[1] * 0.587f + p[2] * 0.114f) * (1.0f / 255.0f);
}

} // namespace

void OsvVulkanStitcher::computeSeamLut(
    const uint8_t* master_rgba,
    size_t master_stride,
    const uint8_t* slave_rgba,
    size_t slave_stride,
    uint32_t sensor_w,
    uint32_t sensor_h,
    const LensCalibration& lens_master,
    const LensCalibration& lens_slave,
    float* out_phis,
    float* out_rigid_omega
) {
    float q_m_inv[4] = {lens_master.quat[0], -lens_master.quat[1], -lens_master.quat[2], -lens_master.quat[3]};
    float q_s_inv[4] = {lens_slave.quat[0], -lens_slave.quat[1], -lens_slave.quat[2], -lens_slave.quat[3]};
    Vec3f aMaster = quatRotate(q_m_inv, {0.0f, 0.0f, 1.0f});
    Vec3f aSlave  = quatRotate(q_s_inv, {0.0f, 0.0f, 1.0f});

    Vec3f nBisector = normVec(aMaster - aSlave);
    Vec3f uSeam = normVec(crossVec(aMaster, aSlave));
    Vec3f vSeam = normVec(crossVec(nBisector, uSeam));

    const int N_KNOTS = 128;
    const int N_CAND = 21;
    const float maxSearch = 6.0f * (3.14159265f / 180.0f);
    float costs[N_KNOTS][N_CAND];
    float lumaM[N_KNOTS][N_CAND];
    float lumaS[N_KNOTS][N_CAND];
    float diffs[N_KNOTS][N_CAND];
    bool validKnot[N_KNOTS][N_CAND];

    #pragma omp parallel for schedule(static)
    for (int k = 0; k < N_KNOTS; ++k) {
        float aK = -3.141592653589793f + float(k) * (2.0f * 3.141592653589793f / float(N_KNOTS));
        for (int c = 0; c < N_CAND; ++c) {
            float candPhi = -maxSearch + float(c) * (2.0f * maxSearch / float(N_CAND - 1));
            float diffAccum = 0.0f, mAccum = 0.0f, sAccum = 0.0f;
            int validCount = 0;
            for (int j = -2; j <= 2; ++j) {
                float aSub = aK + float(j) * (0.75f * 3.14159265f / 180.0f);
                Vec3f dSub = uSeam * std::cos(aSub) + vSeam * std::sin(aSub);
                Vec3f dTest = normVec(dSub * std::cos(candPhi) + nBisector * std::sin(candPhi));
                Vec3f d_m = quatRotate(lens_master.quat, dTest);
                Vec3f d_s = quatRotate(lens_slave.quat, dTest);
                float px_m, py_m, px_s, py_s;
                bool vM = projectLens(d_m, lens_master, sensor_w, sensor_h, px_m, py_m);
                bool vS = projectLens(d_s, lens_slave, sensor_w, sensor_h, px_s, py_s);
                if (vM && vS) {
                    float cM = sampleLuma(master_rgba, master_stride, px_m, py_m);
                    float cS = sampleLuma(slave_rgba, slave_stride, px_s, py_s);
                    diffAccum += std::abs(cM - cS);
                    mAccum += cM;
                    sAccum += cS;
                    validCount++;
                }
            }
            if (validCount >= 3) {
                diffs[k][c] = diffAccum / float(validCount);
                lumaM[k][c] = mAccum / float(validCount);
                lumaS[k][c] = sAccum / float(validCount);
                validKnot[k][c] = true;
            } else {
                diffs[k][c] = 1e6f;
                lumaM[k][c] = 0.0f;
                lumaS[k][c] = 0.0f;
                validKnot[k][c] = false;
            }
        }

        // Detect foreground obstacles across the candidate angles at knot k
        float maxKnotDiff = 0.0f;
        for (int c = 0; c < N_CAND; ++c) {
            if (validKnot[k][c]) {
                maxKnotDiff = std::max(maxKnotDiff, diffs[k][c]);
            }
        }

        // Calculate gradient across candidate angles to heavily penalize crossing sharp edges
        for (int c = 0; c < N_CAND; ++c) {
            if (!validKnot[k][c]) {
                costs[k][c] = 1e6f;
                continue;
            }
            float gradM = 0.0f, gradS = 0.0f;
            if (c > 0 && c < N_CAND - 1) {
                gradM = std::abs(lumaM[k][c + 1] - lumaM[k][c - 1]) * 0.5f;
                gradS = std::abs(lumaS[k][c + 1] - lumaS[k][c - 1]) * 0.5f;
            } else if (c == 0) {
                gradM = std::abs(lumaM[k][1] - lumaM[k][0]);
                gradS = std::abs(lumaS[k][1] - lumaS[k][0]);
            } else {
                gradM = std::abs(lumaM[k][N_CAND - 1] - lumaM[k][N_CAND - 2]);
                gradS = std::abs(lumaS[k][N_CAND - 1] - lumaS[k][N_CAND - 2]);
            }
            float edgePenalty = std::max(gradM, gradS);

            float avgDiff = diffs[k][c];
            float candPhi = -maxSearch + float(c) * (2.0f * maxSearch / float(N_CAND - 1));
            float normPhi = candPhi / maxSearch;

            // Obstacle Repulsion:
            // When maxKnotDiff > 0.12f, a foreground obstacle is present at this knot.
            // Heavily penalize candidate angles where disparity is high, guiding the seam
            // smoothly into regions of minimum optical difference.
            float obstacleCost = 0.0f;
            if (maxKnotDiff > 0.12f) {
                float disparityRatio = avgDiff / maxKnotDiff;
                obstacleCost = 1.8f * (disparityRatio * disparityRatio);
            }

            float centerPref = 0.0005f * normPhi * normPhi;
            float edgeCost = 0.05f * edgePenalty * edgePenalty;
            costs[k][c] = avgDiff * avgDiff + centerPref + obstacleCost + edgeCost;
        }
    }

    // Dynamic Programming across circular loop:
    // Pass 1: Warm-up over N_KNOTS to discover steady-state endpoint seed
    float M[N_KNOTS][N_CAND];
    int backtrack[N_KNOTS][N_CAND];
    for (int c = 0; c < N_CAND; ++c) M[0][c] = costs[0][c];
    const float smooth_step_penalty = 0.025f;

    for (int k = 1; k < N_KNOTS; ++k) {
        for (int c = 0; c < N_CAND; ++c) {
            float best_val = 1e9f;
            int best_prev = c;
            for (int prev_c = 0; prev_c < N_CAND; ++prev_c) {
                int dc = std::abs(c - prev_c);
                if (dc > 3) continue; // enforce smooth C0 continuity across knots
                float step_pen = smooth_step_penalty * float(dc);
                float v = M[k - 1][prev_c] + step_pen;
                if (v < best_val) {
                    best_val = v;
                    best_prev = prev_c;
                }
            }
            M[k][c] = costs[k][c] + best_val;
            backtrack[k][c] = best_prev;
        }
    }

    int best_end_c = 0;
    float min_last = M[N_KNOTS - 1][0];
    for (int c = 1; c < N_CAND; ++c) {
        if (M[N_KNOTS - 1][c] < min_last) {
            min_last = M[N_KNOTS - 1][c];
            best_end_c = c;
        }
    }

    // Pass 2: Cyclic-Constrained DP
    // Initialize knot 0 anchored to best_end_c, and enforce circular wrap closure |c(N-1) - c(0)| <= 3
    for (int c = 0; c < N_CAND; ++c) {
        int dc = std::abs(c - best_end_c);
        if (dc <= 3) {
            M[0][c] = costs[0][c] + smooth_step_penalty * float(dc);
        } else {
            M[0][c] = 1e9f;
        }
    }

    for (int k = 1; k < N_KNOTS; ++k) {
        for (int c = 0; c < N_CAND; ++c) {
            float best_val = 1e9f;
            int best_prev = c;
            for (int prev_c = 0; prev_c < N_CAND; ++prev_c) {
                int dc = std::abs(c - prev_c);
                if (dc > 3) continue;
                float step_pen = smooth_step_penalty * float(dc);
                float v = M[k - 1][prev_c] + step_pen;
                if (v < best_val) {
                    best_val = v;
                    best_prev = prev_c;
                }
            }
            M[k][c] = costs[k][c] + best_val;
            backtrack[k][c] = best_prev;
        }
    }

    // Select endpoint that closes loop with knot 0 anchor
    int final_c = best_end_c;
    float min_closed = 1e9f;
    for (int c = 0; c < N_CAND; ++c) {
        int dc = std::abs(c - best_end_c);
        if (dc <= 3 && M[N_KNOTS - 1][c] < min_closed) {
            min_closed = M[N_KNOTS - 1][c];
            final_c = c;
        }
    }

    int path[N_KNOTS];
    path[N_KNOTS - 1] = final_c;
    for (int k = N_KNOTS - 2; k >= 0; --k) {
        path[k] = backtrack[k + 1][path[k + 1]];
    }

    // Convert candidates to angles and apply 3-tap circular smoothing filter
    float raw_phis[N_KNOTS];
    for (int k = 0; k < N_KNOTS; ++k) {
        int c = path[k];
        raw_phis[k] = -maxSearch + float(c) * (2.0f * maxSearch / float(N_CAND - 1));
    }

    float smoothed_phis[N_KNOTS];
    for (int k = 0; k < N_KNOTS; ++k) {
        int km2 = (k - 2 + N_KNOTS) % N_KNOTS;
        int km1 = (k - 1 + N_KNOTS) % N_KNOTS;
        int kp1 = (k + 1) % N_KNOTS;
        int kp2 = (k + 2) % N_KNOTS;
        smoothed_phis[k] = (1.0f / 16.0f) * raw_phis[km2]
                         + (4.0f / 16.0f) * raw_phis[km1]
                         + (6.0f / 16.0f) * raw_phis[k]
                         + (4.0f / 16.0f) * raw_phis[kp1]
                         + (1.0f / 16.0f) * raw_phis[kp2];
    }

    // 2D local block-matching optical flow along the seam ring:
    // Measures residual 2D angular offset (dPhi, dAlpha) between Master and Slave
    // (covering both distant calibration offsets and near-field parallax) using
    // zero-mean SAD over a 5x5 patch around (aK, phiCut) with coarse-to-fine search.
    const float defaultR = (stitchRadius_ > 0.05f && stitchRadius_ < 100.0f) ? stitchRadius_ : 2.2f;
    auto offsetRay = [](const Vec3f &dirBody, const Vec3f &axisLens, float radius) -> Vec3f {
        if (radius > 0.05f && radius < 100.0f) {
            return normVec(dirBody * radius - axisLens * (0.5f * 0.024f));
        }
        return dirBody;
    };
    auto dirFromSpherical = [&](float a, float phi) -> Vec3f {
        Vec3f dSub = uSeam * std::cos(a) + vSeam * std::sin(a);
        return normVec(dSub * std::cos(phi) + nBisector * std::sin(phi));
    };

    constexpr float kDeg2Rad = 3.141592653589793f / 180.0f;
    const float patchStep = 0.35f * kDeg2Rad;
    float raw_dphi[N_KNOTS];
    float raw_dalpha[N_KNOTS];
    float raw_conf[N_KNOTS];

    #pragma omp parallel for schedule(static)
    for (int k = 0; k < N_KNOTS; ++k) {
        float aK = -3.141592653589793f + float(k) * (2.0f * 3.141592653589793f / float(N_KNOTS));
        float phiK = smoothed_phis[k];

        float patchM[25];
        bool validM[25];
        float sumM = 0.0f;
        int cntM = 0;

        for (int py = -2; py <= 2; ++py) {
            for (int px = -2; px <= 2; ++px) {
                int idx = (py + 2) * 5 + (px + 2);
                Vec3f dBody = dirFromSpherical(aK + float(px) * patchStep, phiK + float(py) * patchStep);
                Vec3f d_m = quatRotate(lens_master.quat, offsetRay(dBody, aMaster, defaultR));
                float u, v;
                if (projectLens(d_m, lens_master, sensor_w, sensor_h, u, v)) {
                    float lum = sampleLuma(master_rgba, master_stride, u, v);
                    patchM[idx] = lum;
                    validM[idx] = true;
                    sumM += lum;
                    cntM++;
                } else {
                    patchM[idx] = 0.0f;
                    validM[idx] = false;
                }
            }
        }

        if (cntM < 16) {
            raw_dphi[k] = 0.0f;
            raw_dalpha[k] = 0.0f;
            raw_conf[k] = 0.0f;
            continue;
        }

        float meanM = sumM / float(cntM);
        float varM = 0.0f;
        for (int i = 0; i < 25; ++i) {
            if (validM[i]) {
                float d = patchM[i] - meanM;
                varM += d * d;
            }
        }
        varM /= float(cntM);
        if (varM < 1.5e-4f) {
            raw_dphi[k] = 0.0f;
            raw_dalpha[k] = 0.0f;
            raw_conf[k] = 0.0f;
            continue;
        }

        auto evalSad = [&](float sPhi, float sAlpha, float &outSad) -> bool {
            float patchS[25];
            bool validS[25];
            float sumS = 0.0f;
            int cntBoth = 0;
            for (int py = -2; py <= 2; ++py) {
                for (int px = -2; px <= 2; ++px) {
                    int idx = (py + 2) * 5 + (px + 2);
                    if (!validM[idx]) {
                        validS[idx] = false;
                        continue;
                    }
                    Vec3f dBody = dirFromSpherical(aK + float(px) * patchStep + sAlpha,
                                                   phiK + float(py) * patchStep + sPhi);
                    Vec3f d_s = quatRotate(lens_slave.quat, offsetRay(dBody, aSlave, defaultR));
                    float u, v;
                    if (projectLens(d_s, lens_slave, sensor_w, sensor_h, u, v)) {
                        float lum = sampleLuma(slave_rgba, slave_stride, u, v);
                        patchS[idx] = lum;
                        validS[idx] = true;
                        sumS += lum;
                        cntBoth++;
                    } else {
                        validS[idx] = false;
                    }
                }
            }
            if (cntBoth < 16)
                return false;

            float meanS = sumS / float(cntBoth);
            float sad = 0.0f;
            for (int i = 0; i < 25; ++i) {
                if (validM[i] && validS[i]) {
                    sad += std::abs((patchM[i] - meanM) - (patchS[i] - meanS));
                }
            }
            outSad = sad / float(cntBoth);
            return true;
        };

        float bestScore = 1e9f;
        float bestSad = 1.0f;
        float bestPhi = 0.0f;
        float bestAlpha = 0.0f;

        // Coarse 15x15 search: [-3.5 deg, +3.5 deg] step 0.5 deg
        const float coarseStep = 0.5f * kDeg2Rad;
        for (int iy = -7; iy <= 7; ++iy) {
            float sPhi = float(iy) * coarseStep;
            for (int ix = -7; ix <= 7; ++ix) {
                float sAlpha = float(ix) * coarseStep;
                float sad = 1.0f;
                if (evalSad(sPhi, sAlpha, sad)) {
                    float reg = 0.0002f * ((sPhi * sPhi + sAlpha * sAlpha) / (kDeg2Rad * kDeg2Rad));
                    float score = sad + reg;
                    if (score < bestScore) {
                        bestScore = score;
                        bestSad = sad;
                        bestPhi = sPhi;
                        bestAlpha = sAlpha;
                    }
                }
            }
        }

        // Fine 3x3 refinement: step 0.16 deg
        const float fineStep = 0.16f * kDeg2Rad;
        float cPhi = bestPhi, cAlpha = bestAlpha;
        for (int iy = -1; iy <= 1; ++iy) {
            for (int ix = -1; ix <= 1; ++ix) {
                if (iy == 0 && ix == 0) continue;
                float sPhi = cPhi + float(iy) * fineStep;
                float sAlpha = cAlpha + float(ix) * fineStep;
                float sad = 1.0f;
                if (evalSad(sPhi, sAlpha, sad)) {
                    float reg = 0.0002f * ((sPhi * sPhi + sAlpha * sAlpha) / (kDeg2Rad * kDeg2Rad));
                    float score = sad + reg;
                    if (score < bestScore) {
                        bestScore = score;
                        bestSad = sad;
                        bestPhi = sPhi;
                        bestAlpha = sAlpha;
                    }
                }
            }
        }

        float texWeight = std::clamp((varM - 1.5e-4f) / 1.0e-3f, 0.0f, 1.0f);
        float matchWeight = std::clamp(1.0f - bestSad / 0.07f, 0.0f, 1.0f);
        raw_dphi[k] = bestPhi;
        raw_dalpha[k] = bestAlpha;
        raw_conf[k] = texWeight * matchWeight;
    }

    // Confidence-weighted circular 5-tap smoothing so low-texture knots inherit neighbor flow
    float smoothed_dphi[N_KNOTS];
    float smoothed_dalpha[N_KNOTS];
    float smoothed_conf[N_KNOTS];
    const float kGauss[5] = {1.0f, 4.0f, 6.0f, 4.0f, 1.0f};
    for (int k = 0; k < N_KNOTS; ++k) {
        float wPhi = 0.0f, wAlpha = 0.0f, wSum = 0.0f, cSum = 0.0f;
        for (int o = -2; o <= 2; ++o) {
            int idx = (k + o + N_KNOTS) % N_KNOTS;
            float g = kGauss[o + 2];
            float cw = g * (raw_conf[idx] + 0.02f);
            wPhi += cw * raw_dphi[idx];
            wAlpha += cw * raw_dalpha[idx];
            wSum += cw;
            cSum += g * raw_conf[idx];
        }
        smoothed_dphi[k] = wPhi / wSum;
        smoothed_dalpha[k] = wAlpha / wSum;
        smoothed_conf[k] = std::clamp(cSum / 16.0f, 0.0f, 1.0f);
    }

    // Resolve knots with temporal moving average to prevent video frame-to-frame jitter
    for (int k = 0; k < N_KNOTS; ++k) {
        float phiOut = smoothed_phis[k];
        float dPhiOut = smoothed_dphi[k];
        float dAlphaOut = smoothed_dalpha[k];
        float confOut = smoothed_conf[k];
        if (has_prev_seam_) {
            phiOut = 0.65f * prev_seam_phis_[k] + 0.35f * smoothed_phis[k];
            dPhiOut = 0.65f * prev_seam_dphi_[k] + 0.35f * smoothed_dphi[k];
            dAlphaOut = 0.65f * prev_seam_dalpha_[k] + 0.35f * smoothed_dalpha[k];
            confOut = 0.65f * prev_seam_conf_[k] + 0.35f * smoothed_conf[k];
        }
        prev_seam_phis_[k] = phiOut;
        prev_seam_dphi_[k] = dPhiOut;
        prev_seam_dalpha_[k] = dAlphaOut;
        prev_seam_conf_[k] = confOut;
        out_phis[4 * k + 0] = phiOut;
        out_phis[4 * k + 1] = dPhiOut;
        out_phis[4 * k + 2] = dAlphaOut;
        out_phis[4 * k + 3] = confOut;
    }
    has_prev_seam_ = true;

    if (out_rigid_omega) {
        // Fit a single global rigid 3D rotation omega = (wx, wy, wz) across all
        // high-confidence seam knots:
        //   (-t_k) . omega = dPhi_k
        //   nBisector . omega = dAlpha_k
        double ATA[3][3] = {{1e-4, 0.0, 0.0}, {0.0, 1e-4, 0.0}, {0.0, 0.0, 1e-4}};
        double ATb[3] = {0.0, 0.0, 0.0};
        for (int k = 0; k < N_KNOTS; ++k) {
            double w = static_cast<double>(out_phis[4 * k + 3]);
            if (w < 0.05) continue;
            float aK = -3.141592653589793f + float(k) * (2.0f * 3.141592653589793f / float(N_KNOTS));
            Vec3f tK = normVec(uSeam * (-std::sin(aK)) + vSeam * std::cos(aK));
            double r1[3] = {-tK.x, -tK.y, -tK.z};
            double b1 = static_cast<double>(out_phis[4 * k + 1]);
            double r2[3] = {nBisector.x, nBisector.y, nBisector.z};
            double b2 = static_cast<double>(out_phis[4 * k + 2]);
            for (int i = 0; i < 3; ++i) {
                ATb[i] += w * (r1[i] * b1 + r2[i] * b2);
                for (int j = 0; j < 3; ++j) {
                    ATA[i][j] += w * (r1[i] * r1[j] + r2[i] * r2[j]);
                }
            }
        }
        // 3x3 Gaussian elimination with partial pivoting
        double M[3][4];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) M[i][j] = ATA[i][j];
            M[i][3] = ATb[i];
        }
        for (int col = 0; col < 3; ++col) {
            int piv = col;
            for (int r = col + 1; r < 3; ++r) {
                if (std::abs(M[r][col]) > std::abs(M[piv][col])) piv = r;
            }
            if (piv != col) {
                for (int j = col; j < 4; ++j) std::swap(M[col][j], M[piv][j]);
            }
            if (std::abs(M[col][col]) < 1e-9) continue;
            for (int r = col + 1; r < 3; ++r) {
                double f = M[r][col] / M[col][col];
                for (int j = col; j < 4; ++j) M[r][j] -= f * M[col][j];
            }
        }
        double x[3] = {0.0, 0.0, 0.0};
        for (int i = 2; i >= 0; --i) {
            if (std::abs(M[i][i]) < 1e-9) continue;
            double s = M[i][3];
            for (int j = i + 1; j < 3; ++j) s -= M[i][j] * x[j];
            x[i] = s / M[i][i];
        }
        // Clamp rigid correction to +-2.5 deg per axis for safety
        const double maxOmega = 2.5 * 3.141592653589793 / 180.0;
        for (int i = 0; i < 3; ++i) {
            out_rigid_omega[i] = static_cast<float>(std::clamp(x[i], -maxOmega, maxOmega));
        }
    }
}

bool OsvVulkanStitcher::stitchFrames(const uint8_t *master_rgba,
                                     const uint8_t *slave_rgba,
                                     uint32_t sensor_w,
                                     uint32_t sensor_h,
                                     const LensCalibration &lens_master,
                                     const LensCalibration &lens_slave,
                                     const float *gyro_quat,
                                     uint32_t out_w,
                                     uint32_t out_h,
                                     uint8_t *out_equirect_rgba)
{
    if (!ensureResources(sensor_w, sensor_h, out_w, out_h)) {
        return false;
    }

    size_t inBytes = sensor_w * sensor_h * 4;
    std::memcpy(mappedInMaster_, master_rgba, inBytes);
    std::memcpy(mappedInSlave_, slave_rgba, inBytes);

    computeSeamLut(master_rgba,
                   (size_t)sensor_w * 4,
                   slave_rgba,
                   (size_t)sensor_w * 4,
                   sensor_w,
                   sensor_h,
                   lens_master,
                   lens_slave,
                   static_cast<float*>(mappedSeamLut_));

    return executeStitchPass(sensor_w,
                             sensor_h,
                             lens_master,
                             lens_slave,
                             gyro_quat,
                             out_w,
                             out_h,
                             out_equirect_rgba);
}

bool OsvVulkanStitcher::stitchSideBySide(const uint8_t *sbs_rgba,
                                         uint32_t sbs_w,
                                         uint32_t sbs_h,
                                         const LensCalibration &lens_master,
                                         const LensCalibration &lens_slave,
                                         const float *gyro_quat,
                                         uint32_t out_w,
                                         uint32_t out_h,
                                         uint8_t *out_equirect_rgba)
{
    uint32_t eye_w = sbs_w / 2;
    uint32_t eye_h = sbs_h;
    if (!ensureResources(eye_w, eye_h, out_w, out_h)) {
        return false;
    }

    size_t row_bytes = (size_t) eye_w * 4;
    size_t sbs_stride = (size_t) sbs_w * 4;
    uint8_t *dst_slave = static_cast<uint8_t *>(mappedInSlave_);
    uint8_t *dst_master = static_cast<uint8_t *>(mappedInMaster_);

    for (uint32_t y = 0; y < eye_h; ++y) {
        const uint8_t *src_row = sbs_rgba + (size_t) y * sbs_stride;
        std::memcpy(dst_slave + (size_t) y * row_bytes, src_row, row_bytes);
        std::memcpy(dst_master + (size_t) y * row_bytes, src_row + row_bytes, row_bytes);
    }

    computeSeamLut(sbs_rgba + (size_t)eye_w * 4,
                   sbs_stride,
                   sbs_rgba,
                   sbs_stride,
                   eye_w,
                   eye_h,
                   lens_master,
                   lens_slave,
                   static_cast<float*>(mappedSeamLut_));

    return executeStitchPass(eye_w,
                             eye_h,
                             lens_master,
                             lens_slave,
                             gyro_quat,
                             out_w,
                             out_h,
                             out_equirect_rgba);
}

bool OsvVulkanStitcher::reframeFrames(const uint8_t *master_rgba,
                                      const uint8_t *slave_rgba,
                                      uint32_t sensor_w,
                                      uint32_t sensor_h,
                                      const LensCalibration &lens_master,
                                      const LensCalibration &lens_slave,
                                      const float *gyro_quat,
                                      const ReframeParams &params,
                                      uint32_t out_w,
                                      uint32_t out_h,
                                      uint8_t *out_rect_rgba)
{
    if (reframe_pipeline_ == VK_NULL_HANDLE) {
        std::cerr << "[OSV-Vulkan] Direct reframe pipeline is not initialized." << std::endl;
        return false;
    }
    if (!ensureResources(sensor_w, sensor_h, out_w, out_h)) {
        return false;
    }

    size_t inBytes = (size_t) sensor_w * sensor_h * 4;
    std::memcpy(mappedInMaster_, master_rgba, inBytes);
    std::memcpy(mappedInSlave_, slave_rgba, inBytes);

    return executeReframePass(sensor_w,
                              sensor_h,
                              lens_master,
                              lens_slave,
                              gyro_quat,
                              params,
                              out_w,
                              out_h,
                              out_rect_rgba,
                              master_rgba,
                              (size_t) sensor_w * 4,
                              slave_rgba,
                              (size_t) sensor_w * 4);
}

bool OsvVulkanStitcher::reframeSideBySide(const uint8_t *sbs_rgba,
                                          uint32_t sbs_w,
                                          uint32_t sbs_h,
                                          const LensCalibration &lens_master,
                                          const LensCalibration &lens_slave,
                                          const float *gyro_quat,
                                          const ReframeParams &params,
                                          uint32_t out_w,
                                          uint32_t out_h,
                                          uint8_t *out_rect_rgba)
{
    if (reframe_pipeline_ == VK_NULL_HANDLE) {
        std::cerr << "[OSV-Vulkan] Direct reframe pipeline is not initialized." << std::endl;
        return false;
    }
    uint32_t eye_w = sbs_w / 2;
    uint32_t eye_h = sbs_h;
    if (!ensureResources(eye_w, eye_h, out_w, out_h)) {
        return false;
    }

    size_t row_bytes = (size_t) eye_w * 4;
    size_t sbs_stride = (size_t) sbs_w * 4;
    uint8_t *dst_slave = static_cast<uint8_t *>(mappedInSlave_);
    uint8_t *dst_master = static_cast<uint8_t *>(mappedInMaster_);

    for (uint32_t y = 0; y < eye_h; ++y) {
        const uint8_t *src_row = sbs_rgba + (size_t) y * sbs_stride;
        std::memcpy(dst_slave + (size_t) y * row_bytes, src_row, row_bytes);
        std::memcpy(dst_master + (size_t) y * row_bytes, src_row + row_bytes, row_bytes);
    }

    return executeReframePass(eye_w,
                              eye_h,
                              lens_master,
                              lens_slave,
                              gyro_quat,
                              params,
                              out_w,
                              out_h,
                              out_rect_rgba,
                              sbs_rgba + row_bytes,
                              sbs_stride,
                              sbs_rgba,
                              sbs_stride);
}

bool OsvVulkanStitcher::executeReframePass(uint32_t sensor_w,
                                           uint32_t sensor_h,
                                           const LensCalibration &lens_master,
                                           const LensCalibration &lens_slave,
                                           const float *gyro_quat,
                                           const ReframeParams &params,
                                           uint32_t out_w,
                                           uint32_t out_h,
                                           uint8_t *out_rect_rgba,
                                           const uint8_t *master_rgba_for_seam,
                                           size_t master_stride,
                                           const uint8_t *slave_rgba_for_seam,
                                           size_t slave_stride)
{

    // Build bigsh0t rotation matrix M = Rz(yaw) * Ry(pitch) * Rx(roll) in column-major mat4
    constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
    float rx = params.roll_deg * kDegToRad;
    float ry = params.pitch_deg * kDegToRad;
    float rz = params.yaw_deg * kDegToRad;
    float cx = std::cos(rx), sx = std::sin(rx);
    float cy = std::cos(ry), sy = std::sin(ry);
    float cz = std::cos(rz), sz = std::sin(rz);

    // M = Rz * Ry * Rx
    float m00 = cz * cy;
    float m01 = cz * sy * sx - sz * cx;
    float m02 = cz * sy * cx + sz * sx;
    float m10 = sz * cy;
    float m11 = sz * sy * sx + cz * cx;
    float m12 = sz * sy * cx - cz * sx;
    float m20 = -sy;
    float m21 = cy * sx;
    float m22 = cy * cx;

    float fov = std::clamp(params.fov_deg, 1.0f, 179.0f);
    float tanHalfFov = std::tan(0.5f * fov * kDegToRad);
    float aspect = static_cast<float>(out_h) / static_cast<float>(out_w);

    // Frustum-to-overlap culling: skip expensive CPU DP seam LUT when the
    // rectilinear viewport lies completely inside a single lens hemisphere.
    auto qRot = [](const float q[4], float vx, float vy, float vz, float& ox, float& oy, float& oz) {
        float qw = q[0], qx = q[1], qy = q[2], qz = q[3];
        float uvx = qy * vz - qz * vy;
        float uvy = qz * vx - qx * vz;
        float uvz = qx * vy - qy * vx;
        float uuvx = qy * uvz - qz * uvy;
        float uuvy = qz * uvx - qx * uvz;
        float uuvz = qx * uvy - qy * uvx;
        ox = vx + 2.0f * (qw * uvx + uuvx);
        oy = vy + 2.0f * (qw * uvy + uuvy);
        oz = vz + 2.0f * (qw * uvz + uuvz);
    };
    float qMInv[4] = {lens_master.quat[0], -lens_master.quat[1], -lens_master.quat[2], -lens_master.quat[3]};
    float qSInv[4] = {lens_slave.quat[0], -lens_slave.quat[1], -lens_slave.quat[2], -lens_slave.quat[3]};
    float amx, amy, amz, asx, asy, asz;
    qRot(qMInv, 0.0f, 0.0f, 1.0f, amx, amy, amz);
    qRot(qSInv, 0.0f, 0.0f, 1.0f, asx, asy, asz);
    float nbx = amx - asx, nby = amy - asy, nbz = amz - asz;
    float nbl = std::sqrt(nbx * nbx + nby * nby + nbz * nbz);
    if (nbl > 1e-6f) {
        nbx /= nbl;
        nby /= nbl;
        nbz /= nbl;
    }

    bool hasGyro = gyro_quat
                   && (std::abs(gyro_quat[0] - 1.0f) > 1e-5f
                       || std::abs(gyro_quat[1]) > 1e-5f
                       || std::abs(gyro_quat[2]) > 1e-5f
                       || std::abs(gyro_quat[3]) > 1e-5f);
    float qGInv[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    if (hasGyro) {
        qGInv[0] = gyro_quat[0];
        qGInv[1] = -gyro_quat[1];
        qGInv[2] = -gyro_quat[2];
        qGInv[3] = -gyro_quat[3];
    }

    float minPhi = 10.0f;
    float maxPhi = -10.0f;
    for (int iy = 0; iy < 5; ++iy) {
        float v = -1.0f + 0.5f * static_cast<float>(iy);
        for (int ix = 0; ix < 5; ++ix) {
            float u = -1.0f + 0.5f * static_cast<float>(ix);
            float ny = u * tanHalfFov;
            float nz = v * tanHalfFov * aspect;
            float invL = 1.0f / std::sqrt(1.0f + ny * ny + nz * nz);
            float vx = invL, vy = ny * invL, vz = nz * invL;
            float wx = m00 * vx + m01 * vy + m02 * vz;
            float wy = m10 * vx + m11 * vy + m12 * vz;
            float wz = m20 * vx + m21 * vy + m22 * vz;
            float dx = -wx, dy = wy, dz = -wz;
            if (hasGyro) {
                qRot(qGInv, dx, dy, dz, dx, dy, dz);
            }
            float d = std::clamp(dx * nbx + dy * nby + dz * nbz, -1.0f, 1.0f);
            float phi = std::asin(d);
            minPhi = std::min(minPhi, phi);
            maxPhi = std::max(maxPhi, phi);
        }
    }

    const float halfOverlapMargin = (0.5f * 14.5f) * kDegToRad + 0.02f;
    const bool needsSeam = !(minPhi > halfOverlapMargin || maxPhi < -halfOverlapMargin);
    if (needsSeam) {
        computeSeamLut(master_rgba_for_seam,
                       master_stride,
                       slave_rgba_for_seam,
                       slave_stride,
                       sensor_w,
                       sensor_h,
                       lens_master,
                       lens_slave,
                       static_cast<float *>(mappedSeamLut_));
    }

    ReframeUniforms ubo{};
    // Column 0
    ubo.viewMatrix[0] = m00;
    ubo.viewMatrix[1] = m10;
    ubo.viewMatrix[2] = m20;
    ubo.viewMatrix[3] = 0.0f;
    // Column 1
    ubo.viewMatrix[4] = m01;
    ubo.viewMatrix[5] = m11;
    ubo.viewMatrix[6] = m21;
    ubo.viewMatrix[7] = 0.0f;
    // Column 2
    ubo.viewMatrix[8] = m02;
    ubo.viewMatrix[9] = m12;
    ubo.viewMatrix[10] = m22;
    ubo.viewMatrix[11] = 0.0f;
    // Column 3
    ubo.viewMatrix[15] = 1.0f;

    ubo.tanHalfFov = tanHalfFov;
    ubo.fisheyeAmount = params.fisheye;
    // Calibration intrinsics (cx, cy, fx, fy) are defined in the 3840x3840 sensor frame
    // regardless of whether the input texture is full 4K or 1024x1024 LRF proxy.
    ubo.sensorWidth = 3840.0f;
    ubo.sensorHeight = 3840.0f;

    std::memcpy(mapped_reframe_ubo_, &ubo, sizeof(ReframeUniforms));

    return executeComputePass(reframe_pipeline_,
                              reframe_pipeline_layout_,
                              reframe_desc_set_,
                              sensor_w,
                              sensor_h,
                              lens_master,
                              lens_slave,
                              gyro_quat,
                              out_w,
                              out_h,
                              out_rect_rgba);
}

void OsvVulkanStitcher::updateReframeDescriptors()
{
    if (reframe_desc_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, reframe_desc_pool_, nullptr);
        reframe_desc_pool_ = VK_NULL_HANDLE;
        reframe_desc_set_ = VK_NULL_HANDLE;
    }

    VkDescriptorPoolSize poolSizes[3]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 3;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    vkCreateDescriptorPool(device_, &poolInfo, nullptr, &reframe_desc_pool_);

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = reframe_desc_pool_;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &reframe_desc_layout_;
    vkAllocateDescriptorSets(device_, &setAllocInfo, &reframe_desc_set_);

    VkDescriptorImageInfo outInfo{};
    outInfo.imageView = viewOut_;
    outInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorImageInfo masterInfo{};
    masterInfo.sampler = sampler_;
    masterInfo.imageView = viewMaster_;
    masterInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo slaveInfo{};
    slaveInfo.sampler = sampler_;
    slaveInfo.imageView = viewSlave_;
    slaveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo seamInfo{};
    seamInfo.sampler = samplerSeam_;
    seamInfo.imageView = viewSeamLut_;
    seamInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = reframe_ubo_;
    uboInfo.offset = 0;
    uboInfo.range = sizeof(ReframeUniforms);

    VkWriteDescriptorSet writes[5]{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = reframe_desc_set_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[0].pImageInfo = &outInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = reframe_desc_set_;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &masterInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = reframe_desc_set_;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].pImageInfo = &slaveInfo;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = reframe_desc_set_;
    writes[3].dstBinding = 3;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].pImageInfo = &seamInfo;

    writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[4].dstSet = reframe_desc_set_;
    writes[4].dstBinding = 4;
    writes[4].descriptorCount = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[4].pBufferInfo = &uboInfo;

    vkUpdateDescriptorSets(device_, 5, writes, 0, nullptr);
}

bool OsvVulkanStitcher::executeStitchPass(uint32_t sensor_w,
                                          uint32_t sensor_h,
                                          const LensCalibration &lens_master,
                                          const LensCalibration &lens_slave,
                                          const float *gyro_quat,
                                          uint32_t out_w,
                                          uint32_t out_h,
                                          uint8_t *out_equirect_rgba)
{
    return executeComputePass(pipeline_,
                              pipeline_layout_,
                              descSet_,
                              sensor_w,
                              sensor_h,
                              lens_master,
                              lens_slave,
                              gyro_quat,
                              out_w,
                              out_h,
                              out_equirect_rgba);
}

bool OsvVulkanStitcher::executeComputePass(VkPipeline target_pipeline,
                                           VkPipelineLayout target_layout,
                                           VkDescriptorSet target_desc_set,
                                           uint32_t sensor_w,
                                           uint32_t sensor_h,
                                           const LensCalibration &lens_master,
                                           const LensCalibration &lens_slave,
                                           const float *gyro_quat,
                                           uint32_t out_w,
                                           uint32_t out_h,
                                           uint8_t *out_rgba)
{
    size_t outBytes = out_w * out_h * 4;

    vkResetCommandBuffer(cmd_, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd_, &beginInfo);

    auto transition = [&](VkImage img,
                          VkImageLayout oldL,
                          VkImageLayout newL,
                          VkAccessFlags srcA,
                          VkAccessFlags dstA,
                          VkPipelineStageFlags srcS,
                          VkPipelineStageFlags dstS) {
        VkImageMemoryBarrier b{};
        b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout = oldL;
        b.newLayout = newL;
        b.srcAccessMask = srcA;
        b.dstAccessMask = dstA;
        b.image = img;
        b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.levelCount = 1;
        b.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd_, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
    };

    transition(imgMaster_,
               VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               0,
               VK_ACCESS_TRANSFER_WRITE_BIT,
               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
               VK_PIPELINE_STAGE_TRANSFER_BIT);
    transition(imgSlave_,
               VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               0,
               VK_ACCESS_TRANSFER_WRITE_BIT,
               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
               VK_PIPELINE_STAGE_TRANSFER_BIT);
    transition(imgSeamLut_,
               VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               0,
               VK_ACCESS_TRANSFER_WRITE_BIT,
               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
               VK_PIPELINE_STAGE_TRANSFER_BIT);
    transition(imgOut_,
               VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_GENERAL,
               0,
               VK_ACCESS_SHADER_WRITE_BIT,
               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {sensor_w, sensor_h, 1};

    vkCmdCopyBufferToImage(cmd_,
                           stageInMaster_,
                           imgMaster_,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);
    vkCmdCopyBufferToImage(cmd_,
                           stageInSlave_,
                           imgSlave_,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);

    VkBufferImageCopy seamRegion{};
    seamRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    seamRegion.imageSubresource.layerCount = 1;
    seamRegion.imageExtent = {128, 1, 1};

    vkCmdCopyBufferToImage(cmd_,
                           stageSeamLut_,
                           imgSeamLut_,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &seamRegion);

    transition(imgMaster_,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
               VK_ACCESS_TRANSFER_WRITE_BIT,
               VK_ACCESS_SHADER_READ_BIT,
               VK_PIPELINE_STAGE_TRANSFER_BIT,
               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    transition(imgSlave_,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
               VK_ACCESS_TRANSFER_WRITE_BIT,
               VK_ACCESS_SHADER_READ_BIT,
               VK_PIPELINE_STAGE_TRANSFER_BIT,
               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    transition(imgSeamLut_,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
               VK_ACCESS_TRANSFER_WRITE_BIT,
               VK_ACCESS_SHADER_READ_BIT,
               VK_PIPELINE_STAGE_TRANSFER_BIT,
               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_COMPUTE, target_pipeline);
    vkCmdBindDescriptorSets(cmd_,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            target_layout,
                            0,
                            1,
                            &target_desc_set,
                            0,
                            nullptr);

    PushConstants pc{};
    // Master lens (Stream 1)
    pc.lens_master_intrinsics[0] = lens_master.fx;
    pc.lens_master_intrinsics[1] = lens_master.fy;
    pc.lens_master_intrinsics[2] = lens_master.cx;
    pc.lens_master_intrinsics[3] = lens_master.cy;
    std::memcpy(pc.lens_master_dist, &lens_master.k1, 4 * sizeof(float));
    std::memcpy(pc.lens_master_quat, lens_master.quat, 4 * sizeof(float));

    // Slave lens (Stream 0)
    pc.lens_slave_intrinsics[0] = lens_slave.fx;
    pc.lens_slave_intrinsics[1] = lens_slave.fy;
    pc.lens_slave_intrinsics[2] = lens_slave.cx;
    pc.lens_slave_intrinsics[3] = lens_slave.cy;
    std::memcpy(pc.lens_slave_dist, &lens_slave.k1, 4 * sizeof(float));
    std::memcpy(pc.lens_slave_quat, lens_slave.quat, 4 * sizeof(float));

    if (gyro_quat) {
        std::memcpy(pc.gyro_quat, gyro_quat, 4 * sizeof(float));
    } else {
        // Identity quaternion in (w, x, y, z): w=1, x=0, y=0, z=0
        pc.gyro_quat[0] = 1.0f;
        pc.gyro_quat[1] = 0.0f;
        pc.gyro_quat[2] = 0.0f;
        pc.gyro_quat[3] = 0.0f;
    }

    pc.packedDims = (out_w & 0xFFFFu) | ((out_h & 0xFFFFu) << 16u);
    pc.stitchRadius = stitchRadius_;
    const uint32_t maxFov100 = 19800u;  // 198.00 deg (full 5-term monotonic FOV)
    const uint32_t overlap100 = 1450u;  // 14.50 deg optical overlap band
    pc.packedFovOverlap = (maxFov100 & 0xFFFFu) | ((overlap100 & 0xFFFFu) << 16u);
    pc.packedK5 = static_cast<uint32_t>(floatToHalf(lens_master.k5))
                  | (static_cast<uint32_t>(floatToHalf(lens_slave.k5)) << 16u);

    vkCmdPushConstants(cmd_,
                       target_layout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0,
                       sizeof(PushConstants),
                       &pc);

    uint32_t groupX = (out_w + 15) / 16;
    uint32_t groupY = (out_h + 15) / 16;
    vkCmdDispatch(cmd_, groupX, groupY, 1);

    transition(imgOut_,
               VK_IMAGE_LAYOUT_GENERAL,
               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
               VK_ACCESS_SHADER_WRITE_BIT,
               VK_ACCESS_TRANSFER_READ_BIT,
               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
               VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy outRegion{};
    outRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    outRegion.imageSubresource.layerCount = 1;
    outRegion.imageExtent = {out_w, out_h, 1};
    vkCmdCopyImageToBuffer(cmd_,
                           imgOut_,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           stageOut_,
                           1,
                           &outRegion);

    vkEndCommandBuffer(cmd_);

    vkResetFences(device_, 1, &fence_);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd_;

    vkQueueSubmit(compute_queue_, 1, &submitInfo, fence_);
    vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);

    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = stageOutMem_;
    mappedRange.offset = 0;
    mappedRange.size = VK_WHOLE_SIZE;
    vkInvalidateMappedMemoryRanges(device_, 1, &mappedRange);

    std::memcpy(out_rgba, mappedOut_, outBytes);
    return true;
}
