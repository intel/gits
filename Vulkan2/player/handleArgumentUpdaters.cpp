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

    auto* pNext = reinterpret_cast<VkBaseInStructure*>(
        const_cast<void*>(static_cast<const void*>(pipelineInfo.pNext)));
    while (pNext != nullptr) {
      if (pNext->sType == VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR) {
        auto* libInfo = reinterpret_cast<VkPipelineLibraryCreateInfoKHR*>(pNext);
        auto* libs = const_cast<VkPipeline*>(libInfo->pLibraries);
        for (uint32_t j = 0; j < libInfo->libraryCount; ++j) {
          GITSKey libKey = reinterpret_cast<uint64_t>(libs[j]);
          if (libKey) {
            libs[j] = reinterpret_cast<VkPipeline>(HandleMapService::Get().GetHandle(libKey));
          } else {
            libs[j] = VK_NULL_HANDLE;
          }
        }
      }
      pNext = const_cast<VkBaseInStructure*>(pNext->pNext);
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

    if (write.descriptorCount > 0) {
      switch (write.descriptorType) {
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        if (write.pImageInfo) {
          for (uint32_t j = 0; j < write.descriptorCount; ++j) {
            VkDescriptorImageInfo& imageInfo =
                const_cast<VkDescriptorImageInfo&>(write.pImageInfo[j]);
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
        break;

      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (write.pBufferInfo) {
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
        break;

      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (write.pTexelBufferView) {
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
        break;

      default:
        break;
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

void UpdateHandle(PlayerManager& manager, PointerArgument<VkDescriptorSetLayoutCreateInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  if (arg.Value->pBindings && arg.Value->bindingCount > 0) {
    for (uint32_t i = 0; i < arg.Value->bindingCount; ++i) {
      VkDescriptorSetLayoutBinding& binding =
          const_cast<VkDescriptorSetLayoutBinding&>(arg.Value->pBindings[i]);
      if (binding.pImmutableSamplers && binding.descriptorCount > 0) {
        for (uint32_t j = 0; j < binding.descriptorCount; ++j) {
          GITSKey key = reinterpret_cast<uint64_t>(binding.pImmutableSamplers[j]);
          if (key) {
            const_cast<VkSampler*>(binding.pImmutableSamplers)[j] =
                reinterpret_cast<VkSampler>(HandleMapService::Get().GetHandle(key));
          } else {
            const_cast<VkSampler*>(binding.pImmutableSamplers)[j] = VK_NULL_HANDLE;
          }
        }
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkImageMemoryRequirementsInfo2>& arg) {
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

void UpdateHandle(PlayerManager& manager,
                  PointerArgument<VkDescriptorUpdateTemplateCreateInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  GITSKey descriptorSetLayoutKey = reinterpret_cast<uint64_t>(arg.Value->descriptorSetLayout);
  if (descriptorSetLayoutKey) {
    arg.Value->descriptorSetLayout = reinterpret_cast<VkDescriptorSetLayout>(
        HandleMapService::Get().GetHandle(descriptorSetLayoutKey));
  } else {
    arg.Value->descriptorSetLayout = VK_NULL_HANDLE;
  }

  GITSKey pipelineLayoutKey = reinterpret_cast<uint64_t>(arg.Value->pipelineLayout);
  if (pipelineLayoutKey) {
    arg.Value->pipelineLayout =
        reinterpret_cast<VkPipelineLayout>(HandleMapService::Get().GetHandle(pipelineLayoutKey));
  } else {
    arg.Value->pipelineLayout = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkDependencyInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  if (arg.Value->pBufferMemoryBarriers && arg.Value->bufferMemoryBarrierCount > 0) {
    for (uint32_t i = 0; i < arg.Value->bufferMemoryBarrierCount; ++i) {
      VkBufferMemoryBarrier2& barrier =
          const_cast<VkBufferMemoryBarrier2&>(arg.Value->pBufferMemoryBarriers[i]);
      GITSKey bufferKey = reinterpret_cast<uint64_t>(barrier.buffer);
      if (bufferKey) {
        barrier.buffer = reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(bufferKey));
      } else {
        barrier.buffer = VK_NULL_HANDLE;
      }
    }
  }

  if (arg.Value->pImageMemoryBarriers && arg.Value->imageMemoryBarrierCount > 0) {
    for (uint32_t i = 0; i < arg.Value->imageMemoryBarrierCount; ++i) {
      VkImageMemoryBarrier2& barrier =
          const_cast<VkImageMemoryBarrier2&>(arg.Value->pImageMemoryBarriers[i]);
      GITSKey imageKey = reinterpret_cast<uint64_t>(barrier.image);
      if (imageKey) {
        barrier.image = reinterpret_cast<VkImage>(HandleMapService::Get().GetHandle(imageKey));
      } else {
        barrier.image = VK_NULL_HANDLE;
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkDependencyInfo>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkDependencyInfo& depInfo = arg.Value[i];

    if (depInfo.pBufferMemoryBarriers && depInfo.bufferMemoryBarrierCount > 0) {
      for (uint32_t j = 0; j < depInfo.bufferMemoryBarrierCount; ++j) {
        VkBufferMemoryBarrier2& barrier =
            const_cast<VkBufferMemoryBarrier2&>(depInfo.pBufferMemoryBarriers[j]);
        GITSKey bufferKey = reinterpret_cast<uint64_t>(barrier.buffer);
        if (bufferKey) {
          barrier.buffer = reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(bufferKey));
        } else {
          barrier.buffer = VK_NULL_HANDLE;
        }
      }
    }

    if (depInfo.pImageMemoryBarriers && depInfo.imageMemoryBarrierCount > 0) {
      for (uint32_t j = 0; j < depInfo.imageMemoryBarrierCount; ++j) {
        VkImageMemoryBarrier2& barrier =
            const_cast<VkImageMemoryBarrier2&>(depInfo.pImageMemoryBarriers[j]);
        GITSKey imageKey = reinterpret_cast<uint64_t>(barrier.image);
        if (imageKey) {
          barrier.image = reinterpret_cast<VkImage>(HandleMapService::Get().GetHandle(imageKey));
        } else {
          barrier.image = VK_NULL_HANDLE;
        }
      }
    }
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkBufferMemoryRequirementsInfo2>& arg) {
  if (!arg.Value) {
    return;
  }
  GITSKey key = reinterpret_cast<uint64_t>(arg.Value->buffer);
  if (key) {
    arg.Value->buffer = reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(key));
  } else {
    arg.Value->buffer = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkBufferDeviceAddressInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  GITSKey key = reinterpret_cast<uint64_t>(arg.Value->buffer);
  if (key) {
    arg.Value->buffer = reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(key));
  } else {
    arg.Value->buffer = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkBufferViewCreateInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  GITSKey key = reinterpret_cast<uint64_t>(arg.Value->buffer);
  if (key) {
    arg.Value->buffer = reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(key));
  } else {
    arg.Value->buffer = VK_NULL_HANDLE;
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkRenderingInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  auto updateAttachment = [](VkRenderingAttachmentInfo& attachment) {
    GITSKey imageViewKey = reinterpret_cast<uint64_t>(attachment.imageView);
    if (imageViewKey) {
      attachment.imageView =
          reinterpret_cast<VkImageView>(HandleMapService::Get().GetHandle(imageViewKey));
    } else {
      attachment.imageView = VK_NULL_HANDLE;
    }

    GITSKey resolveImageViewKey = reinterpret_cast<uint64_t>(attachment.resolveImageView);
    if (resolveImageViewKey) {
      attachment.resolveImageView =
          reinterpret_cast<VkImageView>(HandleMapService::Get().GetHandle(resolveImageViewKey));
    } else {
      attachment.resolveImageView = VK_NULL_HANDLE;
    }
  };

  if (arg.Value->pColorAttachments && arg.Value->colorAttachmentCount > 0) {
    for (uint32_t i = 0; i < arg.Value->colorAttachmentCount; ++i) {
      updateAttachment(const_cast<VkRenderingAttachmentInfo&>(arg.Value->pColorAttachments[i]));
    }
  }

  if (arg.Value->pDepthAttachment) {
    updateAttachment(const_cast<VkRenderingAttachmentInfo&>(*arg.Value->pDepthAttachment));
  }

  if (arg.Value->pStencilAttachment) {
    updateAttachment(const_cast<VkRenderingAttachmentInfo&>(*arg.Value->pStencilAttachment));
  }
}

void UpdateHandle(PlayerManager& manager, PointerArgument<VkMemoryAllocateInfo>& arg) {
  if (!arg.Value) {
    return;
  }

  auto* dedicatedAllocateInfo =
      reinterpret_cast<VkMemoryDedicatedAllocateInfo*>(const_cast<void*>(arg.Value->pNext));
  while (dedicatedAllocateInfo) {
    if (dedicatedAllocateInfo->sType == VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO) {
      GITSKey imageKey = reinterpret_cast<uint64_t>(dedicatedAllocateInfo->image);
      if (imageKey) {
        dedicatedAllocateInfo->image =
            reinterpret_cast<VkImage>(HandleMapService::Get().GetHandle(imageKey));
      } else {
        dedicatedAllocateInfo->image = VK_NULL_HANDLE;
      }

      GITSKey bufferKey = reinterpret_cast<uint64_t>(dedicatedAllocateInfo->buffer);
      if (bufferKey) {
        dedicatedAllocateInfo->buffer =
            reinterpret_cast<VkBuffer>(HandleMapService::Get().GetHandle(bufferKey));
      } else {
        dedicatedAllocateInfo->buffer = VK_NULL_HANDLE;
      }
      break;
    }
    dedicatedAllocateInfo =
        reinterpret_cast<VkMemoryDedicatedAllocateInfo*>(reinterpret_cast<uintptr_t>(
            reinterpret_cast<const VkBaseInStructure*>(dedicatedAllocateInfo)->pNext));
  }
}

void UpdateHandle(PlayerManager& manager, ArrayArgument<VkMappedMemoryRange>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    VkMappedMemoryRange& range = arg.Value[i];
    GITSKey key = reinterpret_cast<uint64_t>(range.memory);
    if (key) {
      range.memory = reinterpret_cast<VkDeviceMemory>(HandleMapService::Get().GetHandle(key));
    } else {
      range.memory = VK_NULL_HANDLE;
    }
  }
}

} // namespace vulkan
} // namespace gits
