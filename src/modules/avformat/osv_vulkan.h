#ifndef OSV_VULKAN_H
#define OSV_VULKAN_H

#include "osv_metadata.h"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct PushConstants
{
    float lens_master_intrinsics[4]; // fx, fy, cx, cy
    float lens_master_dist[4];       // k1, k2, k3, k4
    float lens_master_quat[4];       // w, x, y, z
    float lens_slave_intrinsics[4];  // fx, fy, cx, cy
    float lens_slave_dist[4];        // k1, k2, k3, k4
    float lens_slave_quat[4];        // w, x, y, z
    float gyro_quat[4];              // w, x, y, z
    uint32_t outWidth;
    uint32_t outHeight;
    float maxFovDegrees;
    float blendOverlapDegrees;
};

class OsvVulkanStitcher
{
public:
    OsvVulkanStitcher();
    ~OsvVulkanStitcher();

    bool initialize(const std::string &spv_shader_path = "");
    void cleanup();

    bool stitchFrames(const uint8_t *master_rgba,
                      const uint8_t *slave_rgba,
                      uint32_t sensor_w,
                      uint32_t sensor_h,
                      const LensCalibration &lens_master,
                      const LensCalibration &lens_slave,
                      const float *gyro_quat,
                      uint32_t out_w,
                      uint32_t out_h,
                      uint8_t *out_equirect_rgba);

    bool stitchSideBySide(const uint8_t *sbs_rgba,
                          uint32_t sbs_w,
                          uint32_t sbs_h,
                          const LensCalibration &lens_master,
                          const LensCalibration &lens_slave,
                          const float *gyro_quat,
                          uint32_t out_w,
                          uint32_t out_h,
                          uint8_t *out_equirect_rgba);

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue compute_queue_ = VK_NULL_HANDLE;
    uint32_t queue_family_index_ = 0;

    VkShaderModule shader_module_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout desc_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;

    // Persistent allocated GPU resources
    uint32_t alloc_sensor_w_ = 0;
    uint32_t alloc_sensor_h_ = 0;
    uint32_t alloc_out_w_ = 0;
    uint32_t alloc_out_h_ = 0;

    VkBuffer stageInMaster_ = VK_NULL_HANDLE;
    VkBuffer stageInSlave_ = VK_NULL_HANDLE;
    VkBuffer stageOut_ = VK_NULL_HANDLE;
    VkDeviceMemory stageInMasterMem_ = VK_NULL_HANDLE;
    VkDeviceMemory stageInSlaveMem_ = VK_NULL_HANDLE;
    VkDeviceMemory stageOutMem_ = VK_NULL_HANDLE;
    void *mappedInMaster_ = nullptr;
    void *mappedInSlave_ = nullptr;
    void *mappedOut_ = nullptr;

    VkImage imgMaster_ = VK_NULL_HANDLE;
    VkImage imgSlave_ = VK_NULL_HANDLE;
    VkImage imgOut_ = VK_NULL_HANDLE;
    VkDeviceMemory memMaster_ = VK_NULL_HANDLE;
    VkDeviceMemory memSlave_ = VK_NULL_HANDLE;
    VkDeviceMemory memOut_ = VK_NULL_HANDLE;
    VkImageView viewMaster_ = VK_NULL_HANDLE;
    VkImageView viewSlave_ = VK_NULL_HANDLE;
    VkImageView viewOut_ = VK_NULL_HANDLE;

    VkSampler sampler_ = VK_NULL_HANDLE;
    VkDescriptorPool descPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descSet_ = VK_NULL_HANDLE;
    VkCommandBuffer cmd_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;

    bool initVulkanCore();
    bool createPipeline(const std::string &spv_path);
    bool ensureResources(uint32_t sensor_w, uint32_t sensor_h, uint32_t out_w, uint32_t out_h);
    bool executeStitchPass(uint32_t sensor_w,
                           uint32_t sensor_h,
                           const LensCalibration &lens_master,
                           const LensCalibration &lens_slave,
                           const float *gyro_quat,
                           uint32_t out_w,
                           uint32_t out_h,
                           uint8_t *out_equirect_rgba);
    void freeResources();
};

#endif // OSV_VULKAN_H
