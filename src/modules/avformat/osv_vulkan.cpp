#include "osv_vulkan.h"
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
    if (!initVulkanCore()) {
        std::cerr << "[OSV-Vulkan] Failed to initialize Vulkan core." << std::endl;
        return false;
    }
    if (!createPipeline(spv_shader_path)) {
        std::cerr << "[OSV-Vulkan] Failed to create compute pipeline." << std::endl;
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
    std::ifstream file(spv_path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[OSV-Vulkan] Shader file not found: " << spv_path << std::endl;
        return false;
    }
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = buffer.size();
    moduleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(buffer.data());

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
    createBuf(128 * sizeof(float),
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              stageSeamLut_,
              stageSeamLutMem_);

    vkMapMemory(device_, stageInMasterMem_, 0, inBytes, 0, &mappedInMaster_);
    vkMapMemory(device_, stageInSlaveMem_, 0, inBytes, 0, &mappedInSlave_);
    vkMapMemory(device_, stageOutMem_, 0, outBytes, 0, &mappedOut_);
    vkMapMemory(device_, stageSeamLutMem_, 0, 128 * sizeof(float), 0, &mappedSeamLut_);

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
              VK_FORMAT_R32_SFLOAT);

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
    createView(imgSeamLut_, viewSeamLut_, VK_FORMAT_R32_SFLOAT);

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

static inline float kannalaBrandtRd(float th, const float k[4]) {
    const float th0 = 1.57079632679f;
    if (th <= th0) {
        float t2 = th * th, t4 = t2 * t2, t6 = t4 * t2, t8 = t4 * t4;
        return th * (1.0f + k[0] * t2 + k[1] * t4 + k[2] * t6 + k[3] * t8);
    } else {
        float t2 = th0 * th0, t4 = t2 * t2, t6 = t4 * t2, t8 = t4 * t4;
        float rd0 = th0 * (1.0f + k[0] * t2 + k[1] * t4 + k[2] * t6 + k[3] * t8);
        float drd0 = 1.0f + 3.0f * k[0] * t2 + 5.0f * k[1] * t4 + 7.0f * k[2] * t6 + 9.0f * k[3] * t8;
        return rd0 + drd0 * (th - th0);
    }
}

static inline bool projectLens(const Vec3f& dLens, const LensCalibration& cal, uint32_t w, uint32_t h, float& px, float& py) {
    float rho = std::sqrt(dLens.x * dLens.x + dLens.y * dLens.y);
    float th = std::atan2(rho, dLens.z);
    if (th > 1.70327f) return false; // 97.59 deg
    float rd = kannalaBrandtRd(th, &cal.k1);
    float safe_rho = std::max(rho, 1e-6f);
    px = cal.cx + cal.fx * rd * (dLens.x / safe_rho);
    py = cal.cy + cal.fy * rd * (dLens.y / safe_rho);
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
    float* out_phis
) {
    float q_m_inv[4] = {lens_master.quat[0], -lens_master.quat[1], -lens_master.quat[2], -lens_master.quat[3]};
    float q_s_inv[4] = {lens_slave.quat[0], -lens_slave.quat[1], -lens_slave.quat[2], -lens_slave.quat[3]};
    Vec3f aMaster = quatRotate(q_m_inv, {0.0f, 0.0f, 1.0f});
    Vec3f aSlave  = quatRotate(q_s_inv, {0.0f, 0.0f, 1.0f});

    Vec3f nBisector = normVec(aMaster - aSlave);
    Vec3f uSeam = normVec(crossVec(aMaster, aSlave));
    Vec3f vSeam = normVec(crossVec(nBisector, uSeam));

    const int N_KNOTS = 128;
    const int N_CAND = 17;
    const float maxSearch = 6.2f * (3.14159265f / 180.0f);
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
                if (projectLens(d_m, lens_master, sensor_w, sensor_h, px_m, py_m) &&
                    projectLens(d_s, lens_slave, sensor_w, sensor_h, px_s, py_s)) {
                    float cM = sampleLuma(master_rgba, master_stride, px_m, py_m);
                    float cS = sampleLuma(slave_rgba, slave_stride, px_s, py_s);
                    diffAccum += std::abs(cM - cS);
                    mAccum += cM;
                    sAccum += cS;
                    validCount++;
                }
            }
            if (validCount > 2) {
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
            // Strongly penalize cutting through the obstacle body/edges, and guide the seam
            // towards the Master lens side (candPhi < 0) where optics and lighting are superior.
            float obstacleCost = 0.0f;
            if (maxKnotDiff > 0.12f) {
                float disparityRatio = avgDiff / maxKnotDiff;
                obstacleCost = 0.25f * (disparityRatio * disparityRatio);
                // Forward optical preference for Master lens on obstacles
                if (normPhi > 0.0f) {
                    obstacleCost += 0.08f * normPhi;
                }
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
    const float smooth_step_penalty = 0.020f;

    for (int k = 1; k < N_KNOTS; ++k) {
        for (int c = 0; c < N_CAND; ++c) {
            float best_val = 1e9f;
            int best_prev = c;
            for (int prev_c = 0; prev_c < N_CAND; ++prev_c) {
                int dc = std::abs(c - prev_c);
                if (dc > 2) continue; // enforce smooth C0 continuity across knots
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
    // Initialize knot 0 anchored to best_end_c, and enforce circular wrap closure |c(N-1) - c(0)| <= 2
    for (int c = 0; c < N_CAND; ++c) {
        int dc = std::abs(c - best_end_c);
        if (dc <= 2) {
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
                if (dc > 2) continue;
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
        if (dc <= 2 && M[N_KNOTS - 1][c] < min_closed) {
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
        int prev_k = (k - 1 + N_KNOTS) % N_KNOTS;
        int next_k = (k + 1) % N_KNOTS;
        smoothed_phis[k] = 0.25f * raw_phis[prev_k] + 0.50f * raw_phis[k] + 0.25f * raw_phis[next_k];
    }

    // Resolve knots with temporal moving average to prevent video frame-to-frame seam jitter
    for (int k = 0; k < N_KNOTS; ++k) {
        if (has_prev_seam_) {
            out_phis[k] = 0.65f * prev_seam_phis_[k] + 0.35f * smoothed_phis[k];
        } else {
            out_phis[k] = smoothed_phis[k];
        }
        prev_seam_phis_[k] = out_phis[k];
    }
    has_prev_seam_ = true;
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

bool OsvVulkanStitcher::executeStitchPass(uint32_t sensor_w,
                                          uint32_t sensor_h,
                                          const LensCalibration &lens_master,
                                          const LensCalibration &lens_slave,
                                          const float *gyro_quat,
                                          uint32_t out_w,
                                          uint32_t out_h,
                                          uint8_t *out_equirect_rgba)
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

    vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdBindDescriptorSets(cmd_,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline_layout_,
                            0,
                            1,
                            &descSet_,
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
    pc.maxFovDegrees = 195.18f;
    pc.blendOverlapDegrees = 14.5f; // full optical overlap band

    vkCmdPushConstants(cmd_,
                       pipeline_layout_,
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

    std::memcpy(out_equirect_rgba, mappedOut_, outBytes);
    return true;
}
