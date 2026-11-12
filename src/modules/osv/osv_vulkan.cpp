#include "osv_vulkan.h"
#include "osv_stitch_spv.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

static bool findMemoryType(VkPhysicalDevice physicalDevice,
                           uint32_t typeFilter,
                           VkMemoryPropertyFlags properties,
                           uint32_t &outIndex,
                           bool allowFallback = false,
                           VkMemoryPropertyFlags fallbackProperties = 0)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            outIndex = i;
            return true;
        }
    }
    if (allowFallback) {
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i))
                && (memProperties.memoryTypes[i].propertyFlags & fallbackProperties)
                       == fallbackProperties) {
                outIndex = i;
                return true;
            }
        }
    }
    return false;
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
    const uint32_t *code = nullptr;
    size_t code_size = 0;
    std::vector<char> buffer;

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
        std::cerr << "[OSV-Vulkan] Failed to create compute shader module." << std::endl;
        return false;
    }

    // Descriptor Layout:
    // Binding 0: storage image (output equirectangular)
    // Binding 1: combined image sampler (master lens)
    // Binding 2: combined image sampler (slave lens)
    VkDescriptorSetLayoutBinding bindings[3]{};
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

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
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
                         VkMemoryPropertyFlags fallbackProps = 0) -> bool {
        VkBufferCreateInfo bInfo{};
        bInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bInfo.size = size;
        bInfo.usage = usage;
        bInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device_, &bInfo, nullptr, &buf) != VK_SUCCESS)
            return false;

        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device_, buf, &req);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = req.size;
        if (!findMemoryType(physical_device_,
                            req.memoryTypeBits,
                            props,
                            allocInfo.memoryTypeIndex,
                            allowFallback,
                            fallbackProps))
            return false;
        if (vkAllocateMemory(device_, &allocInfo, nullptr, &mem) != VK_SUCCESS)
            return false;
        return vkBindBufferMemory(device_, buf, mem, 0) == VK_SUCCESS;
    };

    if (!createBuf(inBytes,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   stageInMaster_,
                   stageInMasterMem_)) {
        freeResources();
        return false;
    }
    if (!createBuf(inBytes,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   stageInSlave_,
                   stageInSlaveMem_)) {
        freeResources();
        return false;
    }
    // Prefer HOST_CACHED memory for GPU->CPU readback to avoid 45ms uncached write-combining read stalls on Intel/AMD GPUs
    if (!createBuf(outBytes,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                   stageOut_,
                   stageOutMem_,
                   true,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        freeResources();
        return false;
    }

    if (vkMapMemory(device_, stageInMasterMem_, 0, inBytes, 0, &mappedInMaster_) != VK_SUCCESS
        || vkMapMemory(device_, stageInSlaveMem_, 0, inBytes, 0, &mappedInSlave_) != VK_SUCCESS
        || vkMapMemory(device_, stageOutMem_, 0, VK_WHOLE_SIZE, 0, &mappedOut_) != VK_SUCCESS) {
        freeResources();
        return false;
    }

    auto createImg = [&](uint32_t w,
                         uint32_t h,
                         VkImageUsageFlags usage,
                         VkImage &img,
                         VkDeviceMemory &mem) -> bool {
        VkImageCreateInfo iInfo{};
        iInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        iInfo.imageType = VK_IMAGE_TYPE_2D;
        iInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        iInfo.extent = {w, h, 1};
        iInfo.mipLevels = 1;
        iInfo.arrayLayers = 1;
        iInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        iInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        iInfo.usage = usage;
        iInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        iInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkCreateImage(device_, &iInfo, nullptr, &img) != VK_SUCCESS)
            return false;

        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(device_, img, &req);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = req.size;
        if (!findMemoryType(physical_device_,
                            req.memoryTypeBits,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            allocInfo.memoryTypeIndex))
            return false;
        if (vkAllocateMemory(device_, &allocInfo, nullptr, &mem) != VK_SUCCESS)
            return false;
        return vkBindImageMemory(device_, img, mem, 0) == VK_SUCCESS;
    };

    if (!createImg(sensor_w,
                   sensor_h,
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                   imgMaster_,
                   memMaster_)
        || !createImg(sensor_w,
                      sensor_h,
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      imgSlave_,
                      memSlave_)
        || !createImg(out_w,
                      out_h,
                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                      imgOut_,
                      memOut_)) {
        freeResources();
        return false;
    }

    auto createView = [&](VkImage img, VkImageView &view) -> bool {
        VkImageViewCreateInfo vInfo{};
        vInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vInfo.image = img;
        vInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        vInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vInfo.subresourceRange.levelCount = 1;
        vInfo.subresourceRange.layerCount = 1;
        return vkCreateImageView(device_, &vInfo, nullptr, &view) == VK_SUCCESS;
    };

    if (!createView(imgMaster_, viewMaster_) || !createView(imgSlave_, viewSlave_)
        || !createView(imgOut_, viewOut_)) {
        freeResources();
        return false;
    }

    VkSamplerCreateInfo sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sInfo.magFilter = VK_FILTER_LINEAR;
    sInfo.minFilter = VK_FILTER_LINEAR;
    sInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(device_, &sInfo, nullptr, &sampler_) != VK_SUCCESS) {
        freeResources();
        return false;
    }

    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 2;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descPool_) != VK_SUCCESS) {
        freeResources();
        return false;
    }

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = descPool_;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &desc_layout_;
    if (vkAllocateDescriptorSets(device_, &setAllocInfo, &descSet_) != VK_SUCCESS) {
        freeResources();
        return false;
    }

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

    VkWriteDescriptorSet writes[3]{};
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

    vkUpdateDescriptorSets(device_, 3, writes, 0, nullptr);

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = command_pool_;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(device_, &cmdAllocInfo, &cmd_) != VK_SUCCESS) {
        freeResources();
        return false;
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(device_, &fenceInfo, nullptr, &fence_) != VK_SUCCESS) {
        freeResources();
        return false;
    }

    return true;
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

    pc.outWidth = out_w;
    pc.outHeight = out_h;
    pc.maxFovDegrees = 195.18f;
    pc.blendOverlapDegrees = 10.0f;

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

    VkResult submitRes = vkQueueSubmit(compute_queue_, 1, &submitInfo, fence_);
    if (submitRes != VK_SUCCESS) {
        return false;
    }

    VkResult waitRes = vkWaitForFences(device_, 1, &fence_, VK_TRUE, 5000000000ULL);
    if (waitRes != VK_SUCCESS) {
        return false;
    }

    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = stageOutMem_;
    mappedRange.offset = 0;
    mappedRange.size = VK_WHOLE_SIZE;
    vkInvalidateMappedMemoryRanges(device_, 1, &mappedRange);

    std::memcpy(out_equirect_rgba, mappedOut_, outBytes);
    return true;
}
