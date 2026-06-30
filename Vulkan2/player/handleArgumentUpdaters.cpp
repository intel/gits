// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "handleArgumentUpdaters.h"

namespace gits {
namespace vulkan {

void UpdateHandle(PlayerManager& manager, PointerArgument<VkCommandBufferAllocateInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  GITSKey key = reinterpret_cast<uint64_t>(arg.Value->commandPool);
  if (key) {
    arg.Value->commandPool =
        reinterpret_cast<VkCommandPool>(HandleMapService::Get().GetHandle(key));
  } else {
    arg.Value->commandPool = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkDescriptorSetAllocateInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  uint64_t key = reinterpret_cast<uint64_t>(arg.Value->descriptorPool);
  if (key) {
    arg.Value->descriptorPool =
        reinterpret_cast<VkDescriptorPool>(HandleMapService::Get().GetHandle(key));
  } else {
    arg.Value->descriptorPool = VK_NULL_HANDLE;
  }

  if (arg.Value->pSetLayouts && arg.Value->descriptorSetCount > 0) {
    for (uint32_t i = 0; i < arg.Value->descriptorSetCount; ++i) {
      GITSKey key = reinterpret_cast<uint64_t>(arg.Value->pSetLayouts[i]);
      if (key) {
        const_cast<VkDescriptorSetLayout*>(arg.Value->pSetLayouts)[i] =
            reinterpret_cast<VkDescriptorSetLayout>(HandleMapService::Get().GetHandle(key));
      } else {
        const_cast<VkDescriptorSetLayout*>(arg.Value->pSetLayouts)[i] = VK_NULL_HANDLE;
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkImageViewCreateInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  GITSKey key = reinterpret_cast<uint64_t>(arg.Value->image);
  if (key) {
    arg.Value->image = reinterpret_cast<VkImage>(HandleMapService::Get().GetHandle(key));
  } else {
    arg.Value->image = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkPipelineLayoutCreateInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  if (arg.Value->pSetLayouts && arg.Value->setLayoutCount > 0) {
    for (uint32_t i = 0; i < arg.Value->setLayoutCount; ++i) {
      GITSKey key = reinterpret_cast<uint64_t>(arg.Value->pSetLayouts[i]);
      if (key) {
        const_cast<VkDescriptorSetLayout*>(arg.Value->pSetLayouts)[i] =
            reinterpret_cast<VkDescriptorSetLayout>(HandleMapService::Get().GetHandle(key));
      } else {
        const_cast<VkDescriptorSetLayout*>(arg.Value->pSetLayouts)[i] = VK_NULL_HANDLE;
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkGraphicsPipelineCreateInfo>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkGraphicsPipelineCreateInfo& pipelineInfo = arg.Value[i];
    GITSKey layoutKey = reinterpret_cast<uint64_t>(pipelineInfo.layout);
    if (layoutKey) {
      pipelineInfo.layout =
          reinterpret_cast<VkPipelineLayout>(HandleMapService::Get().GetHandle(layoutKey));
    } else {
      pipelineInfo.layout = VK_NULL_HANDLE;
    }

    GITSKey renderPassKey = reinterpret_cast<uint64_t>(pipelineInfo.renderPass);
    if (renderPassKey) {
      pipelineInfo.renderPass =
          reinterpret_cast<VkRenderPass>(HandleMapService::Get().GetHandle(renderPassKey));
    } else {
      pipelineInfo.renderPass = VK_NULL_HANDLE;
    }

    GITSKey basePipelineKey = reinterpret_cast<uint64_t>(pipelineInfo.basePipelineHandle);
    if (basePipelineKey) {
      pipelineInfo.basePipelineHandle =
          reinterpret_cast<VkPipeline>(HandleMapService::Get().GetHandle(basePipelineKey));
    } else {
      pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    }

    if (pipelineInfo.pStages && pipelineInfo.stageCount > 0) {
      for (uint32_t j = 0; j < pipelineInfo.stageCount; ++j) {
        VkPipelineShaderStageCreateInfo& stage =
            const_cast<VkPipelineShaderStageCreateInfo&>(pipelineInfo.pStages[j]);
        uint64_t shaderModuleKey = reinterpret_cast<uint64_t>(stage.module);
        if (shaderModuleKey) {
          stage.module =
              reinterpret_cast<VkShaderModule>(HandleMapService::Get().GetHandle(shaderModuleKey));
        } else {
          stage.module = VK_NULL_HANDLE;
        }
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkComputePipelineCreateInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  GITSKey layoutKey = reinterpret_cast<uint64_t>(arg.Value->layout);
  if (layoutKey) {
    arg.Value->layout =
        reinterpret_cast<VkPipelineLayout>(HandleMapService::Get().GetHandle(layoutKey));
  } else {
    arg.Value->layout = VK_NULL_HANDLE;
  }

  GITSKey basePipelineKey = reinterpret_cast<uint64_t>(arg.Value->basePipelineHandle);
  if (basePipelineKey) {
    arg.Value->basePipelineHandle =
        reinterpret_cast<VkPipeline>(HandleMapService::Get().GetHandle(basePipelineKey));
  } else {
    arg.Value->basePipelineHandle = VK_NULL_HANDLE;
  }

  GITSKey shaderModuleKey = reinterpret_cast<uint64_t>(arg.Value->stage.module);
  if (shaderModuleKey) {
    arg.Value->stage.module =
        reinterpret_cast<VkShaderModule>(HandleMapService::Get().GetHandle(shaderModuleKey));
  } else {
    arg.Value->stage.module = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkComputePipelineCreateInfo>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkComputePipelineCreateInfo& pipelineInfo = arg.Value[i];

    GITSKey layoutKey = reinterpret_cast<uint64_t>(pipelineInfo.layout);
    if (layoutKey) {
      pipelineInfo.layout =
          reinterpret_cast<VkPipelineLayout>(HandleMapService::Get().GetHandle(layoutKey));
    } else {
      pipelineInfo.layout = VK_NULL_HANDLE;
    }

    GITSKey basePipelineKey = reinterpret_cast<uint64_t>(pipelineInfo.basePipelineHandle);
    if (basePipelineKey) {
      pipelineInfo.basePipelineHandle =
          reinterpret_cast<VkPipeline>(HandleMapService::Get().GetHandle(basePipelineKey));
    } else {
      pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    }

    GITSKey shaderModuleKey = reinterpret_cast<uint64_t>(pipelineInfo.stage.module);
    if (shaderModuleKey) {
      pipelineInfo.stage.module =
          reinterpret_cast<VkShaderModule>(HandleMapService::Get().GetHandle(shaderModuleKey));
    } else {
      pipelineInfo.stage.module = VK_NULL_HANDLE;
    }
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkImageMemoryBarrier>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkImageMemoryBarrier& barrier = arg.Value[i];
    GITSKey imageKey = reinterpret_cast<uint64_t>(barrier.image);
    if (imageKey) {
      barrier.image = reinterpret_cast<VkImage>(HandleMapService::Get().GetHandle(imageKey));
    } else {
      barrier.image = VK_NULL_HANDLE;
    }
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkBufferMemoryBarrier>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkBufferMemoryBarrier& barrier = arg.Value[i];
    GITSKey bufferKey = reinterpret_cast<uint64_t>(barrier.buffer);
    if (bufferKey) {
      barrier.buffer = reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(bufferKey));
    } else {
      barrier.buffer = VK_NULL_HANDLE;
    }
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkWriteDescriptorSet>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkWriteDescriptorSet& write = arg.Value[i];
    GITSKey dstSetKey = reinterpret_cast<uint64_t>(write.dstSet);
    if (dstSetKey) {
      write.dstSet =
          reinterpret_cast<VkDescriptorSet>(HandleMapService::Get().GetHandle(dstSetKey));
    } else {
      write.dstSet = VK_NULL_HANDLE;
    }

    if (write.pBufferInfo && write.descriptorCount > 0) {
      for (uint32_t j = 0; j < write.descriptorCount; ++j) {
        VkDescriptorBufferInfo& bufferInfo =
            const_cast<VkDescriptorBufferInfo&>(write.pBufferInfo[j]);
        GITSKey bufferKey = reinterpret_cast<uint64_t>(bufferInfo.buffer);
        if (bufferKey) {
          bufferInfo.buffer =
              reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(bufferKey));
        } else {
          bufferInfo.buffer = VK_NULL_HANDLE;
        }
      }
    }

    if (write.pImageInfo && write.descriptorCount > 0) {
      for (uint32_t j = 0; j < write.descriptorCount; ++j) {
        VkDescriptorImageInfo& imageInfo = const_cast<VkDescriptorImageInfo&>(write.pImageInfo[j]);
        GITSKey imageViewKey = reinterpret_cast<uint64_t>(imageInfo.imageView);
        if (imageViewKey) {
          imageInfo.imageView =
              reinterpret_cast<VkImageView>(HandleMapService::Get().GetHandle(imageViewKey));
        } else {
          imageInfo.imageView = VK_NULL_HANDLE;
        }

        GITSKey samplerKey = reinterpret_cast<uint64_t>(imageInfo.sampler);
        if (samplerKey) {
          imageInfo.sampler =
              reinterpret_cast<VkSampler>(HandleMapService::Get().GetHandle(samplerKey));
        } else {
          imageInfo.sampler = VK_NULL_HANDLE;
        }
      }
    }

    if (write.pTexelBufferView && write.descriptorCount > 0) {
      for (uint32_t j = 0; j < write.descriptorCount; ++j) {
        VkBufferView& texelBufferView = const_cast<VkBufferView&>(write.pTexelBufferView[j]);
        GITSKey bufferViewKey = reinterpret_cast<uint64_t>(texelBufferView);
        if (bufferViewKey) {
          texelBufferView =
              reinterpret_cast<VkBufferView>(HandleMapService::Get().GetHandle(bufferViewKey));
        } else {
          texelBufferView = VK_NULL_HANDLE;
        }
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkCopyDescriptorSet>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkCopyDescriptorSet& copy = arg.Value[i];
    GITSKey srcSetKey = reinterpret_cast<uint64_t>(copy.srcSet);
    if (srcSetKey) {
      copy.srcSet = reinterpret_cast<VkDescriptorSet>(HandleMapService::Get().GetHandle(srcSetKey));
    } else {
      copy.srcSet = VK_NULL_HANDLE;
    }
    GITSKey dstSetKey = reinterpret_cast<uint64_t>(copy.dstSet);
    if (dstSetKey) {
      copy.dstSet = reinterpret_cast<VkDescriptorSet>(HandleMapService::Get().GetHandle(dstSetKey));
    } else {
      copy.dstSet = VK_NULL_HANDLE;
    }
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkSubmitInfo>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }
  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkSubmitInfo& submitInfo = arg.Value[i];

    if (submitInfo.pCommandBuffers && submitInfo.commandBufferCount > 0) {
      for (uint32_t j = 0; j < submitInfo.commandBufferCount; ++j) {
        GITSKey commandBufferKey = reinterpret_cast<uint64_t>(submitInfo.pCommandBuffers[j]);
        if (commandBufferKey) {
          const_cast<VkCommandBuffer*>(submitInfo.pCommandBuffers)[j] =
              reinterpret_cast<VkCommandBuffer>(
                  HandleMapService::Get().GetHandle(commandBufferKey));
        } else {
          const_cast<VkCommandBuffer*>(submitInfo.pCommandBuffers)[j] = VK_NULL_HANDLE;
        }
      }
    }

    if (submitInfo.pWaitSemaphores && submitInfo.waitSemaphoreCount > 0) {
      for (uint32_t j = 0; j < submitInfo.waitSemaphoreCount; ++j) {
        GITSKey semaphoreKey = reinterpret_cast<uint64_t>(submitInfo.pWaitSemaphores[j]);
        if (semaphoreKey) {
          const_cast<VkSemaphore*>(submitInfo.pWaitSemaphores)[j] =
              reinterpret_cast<VkSemaphore>(HandleMapService::Get().GetHandle(semaphoreKey));
        } else {
          const_cast<VkSemaphore*>(submitInfo.pWaitSemaphores)[j] = VK_NULL_HANDLE;
        }
      }
    }

    if (submitInfo.pSignalSemaphores && submitInfo.signalSemaphoreCount > 0) {
      for (uint32_t j = 0; j < submitInfo.signalSemaphoreCount; ++j) {
        GITSKey semaphoreKey = reinterpret_cast<uint64_t>(submitInfo.pSignalSemaphores[j]);
        if (semaphoreKey) {
          const_cast<VkSemaphore*>(submitInfo.pSignalSemaphores)[j] =
              reinterpret_cast<VkSemaphore>(HandleMapService::Get().GetHandle(semaphoreKey));
        } else {
          const_cast<VkSemaphore*>(submitInfo.pSignalSemaphores)[j] = VK_NULL_HANDLE;
        }
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkSubmitInfo2>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkSubmitInfo2& submitInfo = arg.Value[i];

    if (submitInfo.pCommandBufferInfos && submitInfo.commandBufferInfoCount > 0) {
      for (uint32_t j = 0; j < submitInfo.commandBufferInfoCount; ++j) {
        VkCommandBufferSubmitInfo& cmdBufferInfo =
            const_cast<VkCommandBufferSubmitInfo&>(submitInfo.pCommandBufferInfos[j]);
        GITSKey commandBufferKey = reinterpret_cast<uint64_t>(cmdBufferInfo.commandBuffer);
        if (commandBufferKey) {
          cmdBufferInfo.commandBuffer = reinterpret_cast<VkCommandBuffer>(
              HandleMapService::Get().GetHandle(commandBufferKey));
        } else {
          cmdBufferInfo.commandBuffer = VK_NULL_HANDLE;
        }
      }
    }

    if (submitInfo.pWaitSemaphoreInfos && submitInfo.waitSemaphoreInfoCount > 0) {
      for (uint32_t j = 0; j < submitInfo.waitSemaphoreInfoCount; ++j) {
        VkSemaphoreSubmitInfo& semaphoreInfo =
            const_cast<VkSemaphoreSubmitInfo&>(submitInfo.pWaitSemaphoreInfos[j]);
        GITSKey semaphoreKey = reinterpret_cast<uint64_t>(semaphoreInfo.semaphore);
        if (semaphoreKey) {
          semaphoreInfo.semaphore =
              reinterpret_cast<VkSemaphore>(HandleMapService::Get().GetHandle(semaphoreKey));
        } else {
          semaphoreInfo.semaphore = VK_NULL_HANDLE;
        }
      }
    }

    if (submitInfo.pSignalSemaphoreInfos && submitInfo.signalSemaphoreInfoCount > 0) {
      for (uint32_t j = 0; j < submitInfo.signalSemaphoreInfoCount; ++j) {
        VkSemaphoreSubmitInfo& semaphoreInfo =
            const_cast<VkSemaphoreSubmitInfo&>(submitInfo.pSignalSemaphoreInfos[j]);
        GITSKey semaphoreKey = reinterpret_cast<uint64_t>(semaphoreInfo.semaphore);
        if (semaphoreKey) {
          semaphoreInfo.semaphore =
              reinterpret_cast<VkSemaphore>(HandleMapService::Get().GetHandle(semaphoreKey));
        } else {
          semaphoreInfo.semaphore = VK_NULL_HANDLE;
        }
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkSwapchainCreateInfoKHR>& arg) {
  if (!arg.Value) {
    return;
  }
  GITSKey surfaceKey = reinterpret_cast<uint64_t>(arg.Value->surface);
  if (surfaceKey) {
    arg.Value->surface =
        reinterpret_cast<VkSurfaceKHR>(HandleMapService::Get().GetHandle(surfaceKey));
  } else {
    arg.Value->surface = VK_NULL_HANDLE;
  }

  GITSKey oldSwapchainKey = reinterpret_cast<uint64_t>(arg.Value->oldSwapchain);
  if (oldSwapchainKey) {
    arg.Value->oldSwapchain =
        reinterpret_cast<VkSwapchainKHR>(HandleMapService::Get().GetHandle(oldSwapchainKey));
  } else {
    arg.Value->oldSwapchain = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkSwapchainCreateInfoKHR>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkSwapchainCreateInfoKHR& swapchainInfo = arg.Value[i];
    GITSKey surfaceKey = reinterpret_cast<uint64_t>(swapchainInfo.surface);
    if (surfaceKey) {
      swapchainInfo.surface =
          reinterpret_cast<VkSurfaceKHR>(HandleMapService::Get().GetHandle(surfaceKey));
    } else {
      swapchainInfo.surface = VK_NULL_HANDLE;
    }
    GITSKey oldSwapchainKey = reinterpret_cast<uint64_t>(swapchainInfo.oldSwapchain);
    if (oldSwapchainKey) {
      swapchainInfo.oldSwapchain =
          reinterpret_cast<VkSwapchainKHR>(HandleMapService::Get().GetHandle(oldSwapchainKey));
    } else {
      swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
    }
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkFramebufferCreateInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  GITSKey renderPassKey = reinterpret_cast<uint64_t>(arg.Value->renderPass);
  if (renderPassKey) {
    arg.Value->renderPass =
        reinterpret_cast<VkRenderPass>(HandleMapService::Get().GetHandle(renderPassKey));
  } else {
    arg.Value->renderPass = VK_NULL_HANDLE;
  }

  if (arg.Value->pAttachments && arg.Value->attachmentCount > 0) {
    for (uint32_t i = 0; i < arg.Value->attachmentCount; ++i) {
      GITSKey imageViewKey = reinterpret_cast<uint64_t>(arg.Value->pAttachments[i]);
      if (imageViewKey) {
        const_cast<VkImageView*>(arg.Value->pAttachments)[i] =
            reinterpret_cast<VkImageView>(HandleMapService::Get().GetHandle(imageViewKey));
      } else {
        const_cast<VkImageView*>(arg.Value->pAttachments)[i] = VK_NULL_HANDLE;
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkRenderPassBeginInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  GITSKey renderPassKey = reinterpret_cast<uint64_t>(arg.Value->renderPass);
  if (renderPassKey) {
    arg.Value->renderPass =
        reinterpret_cast<VkRenderPass>(HandleMapService::Get().GetHandle(renderPassKey));
  } else {
    arg.Value->renderPass = VK_NULL_HANDLE;
  }

  GITSKey framebufferKey = reinterpret_cast<uint64_t>(arg.Value->framebuffer);
  if (framebufferKey) {
    arg.Value->framebuffer =
        reinterpret_cast<VkFramebuffer>(HandleMapService::Get().GetHandle(framebufferKey));
  } else {
    arg.Value->framebuffer = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkPresentInfoKHR>& arg) {
  if (!arg.Value) {
    return;
  }
  if (arg.Value->pWaitSemaphores && arg.Value->waitSemaphoreCount > 0) {
    for (uint32_t i = 0; i < arg.Value->waitSemaphoreCount; ++i) {
      GITSKey semaphoreKey = reinterpret_cast<uint64_t>(arg.Value->pWaitSemaphores[i]);
      if (semaphoreKey) {
        const_cast<VkSemaphore*>(arg.Value->pWaitSemaphores)[i] =
            reinterpret_cast<VkSemaphore>(HandleMapService::Get().GetHandle(semaphoreKey));
      } else {
        const_cast<VkSemaphore*>(arg.Value->pWaitSemaphores)[i] = VK_NULL_HANDLE;
      }
    }
  }

  if (arg.Value->pSwapchains && arg.Value->swapchainCount > 0) {
    for (uint32_t i = 0; i < arg.Value->swapchainCount; ++i) {
      GITSKey swapchainKey = reinterpret_cast<uint64_t>(arg.Value->pSwapchains[i]);
      if (swapchainKey) {
        const_cast<VkSwapchainKHR*>(arg.Value->pSwapchains)[i] =
            reinterpret_cast<VkSwapchainKHR>(HandleMapService::Get().GetHandle(swapchainKey));
      } else {
        const_cast<VkSwapchainKHR*>(arg.Value->pSwapchains)[i] = VK_NULL_HANDLE;
      }
    }
  }
}

} // namespace vulkan
} // namespace gits
