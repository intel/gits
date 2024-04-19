-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2024 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- LevelZero performance dump in old GITS format.
-- Dumps GPU profiling info for NDRangeKernel.

local info = debug.getinfo(1, 'S')                   -- get source file name
local dir = string.match(info.source, '[^@]*/')      -- get directory part
if dir == nil then
    dir = '.'
end
ossep = package.config:sub(1,1)
dir = string.gsub(dir, '/', ossep)                   -- replace / with OS Separator
package.path = package.path .. ';' .. dir .. '?.lua' -- append to package.path
require 'l0Constants'

-- LUA GLOBALS --

PTR_SIZE = gits.getPtrSize()

MAX_NUMBER_OF_EVENTS_PER_EVENT_POOL = 1024  -- Maximum number of events within the pool
EVENT_INDEX = 0                             -- Index of created event in the GITS event pool
EVENT_POOL_HANDLE = nil                     -- Global GITS handle of ze_event_pool_handle_t
GITS_EVENTS = {}                            -- List of events created by script to be deleted at the end


SUBMIT_QUEUE_NUMBER = 0                     -- Global id of GITS kernel nomenclature(X/Y/Z) -> X
COMMAND_LIST_NUMBER = 0                     -- Global id of GITS kernel nomenclature(X/Y/Z) -> Y
APPEND_LAUNCH_KERNEL_NUMBER = 0             -- Global id of GITS kernel nomenclature(X/Y/Z) -> Z

EVENT_LIST = {}                             -- State tracking of event objects
COMMAND_LIST = {}                           -- State tracking of command list objects
COMMAND_QUEUE_LIST = {}                     -- State tracking of command queue objects
KERNEL_LIST = {}                            -- State tracking of kernel objects

TIMER_RESOLUTION = 0                        -- Resolution of device timer used for profiling, timestamps. Units are in nanoseconds.
KERNEL_TIMESTAMP_VALID_BITS = 0             -- Number of valid bits in the kernel timestamp values
UINT64_MAX = 0xFFFFFFFF                     -- Max uint64 value

Event = {                                   -- Event object info
  handle = 0,                               -- Event handle
  kernel = nil,                             -- Kernel object reference
  hCommandList = 0,                         -- Command list handle
  kernelStart = 0,                          -- (context)device clock at start of kernel execution
  kernelEnd = 0,                            -- (context)device clock at end of kernel execution
  cqNumber = 0,                             -- Id of command queue submit
  cmdListNumber = 0,                        -- Id of command list creation
  kernelLaunchNumber = 0,                   -- Id of command list append kernel launch
  isCompleted = false,                      -- Flag responsible for filtering fully initialized events
  appendTimestampResult = {},               -- L0 object structure ze_kernel_timestamp_result_t
  gatheredData = false                      -- Determines whether application uses AppendEventResets
}

CommandList = {                             -- Command list object info
  handle = 0,                               -- Command list handle
  number = 0,                               -- Id of command list creation
  events = {},                              -- List of references to kernel events
  isCompleted = false                       -- Determines whether list has registered its every event
}

CommandQueue = {                            -- Command queue submit object info
  handle = 0,                               -- Command queue handle
  number = 0,                               -- Id of command queue submit
  hFence = nil,                             -- Fence handle of command queue submit
  cmdLists = {},                            -- Command lists listed for submit
  isCompleted = false                       -- Determines whether queue submit has registered its every event
}

Kernel = {                                  -- Kernel object info
  handle = 0,                               -- Kernel handle
  name = '',                                -- Kernel name
  group_size_x = 0,                         -- group size for X dimension to use for this kernel
  group_size_y = 0,                         -- group size for Y dimension to use for this kernel
  group_size_z = 0                          -- group size for Z dimension to use for this kernel
}

CSV_HEADER = {'launch_no', 'kernel', 'start_ns', 'complete_ns', 'duration_ns', 'gws'}

-- LUA STATE DYNAMIC OBJECTS --

function Event:new(hKernel, hEvent, hCommandList, kernelLaunchNumber)
  local ev = {}
  setmetatable(ev, {__index = self})
  ev.kernel = GetObject(KERNEL_LIST, hKernel)
  ev.handle = hEvent
  ev.hCommandList = hCommandList
  ev.cmdListNumber = 0
  for i=1, COMMAND_LIST_NUMBER do
   if ev.hCommandList == COMMAND_LIST[i].handle then
    ev.cmdListNumber = COMMAND_LIST[i].number
   end
  end
  ev.kernelStart = 0
  ev.kernelEnd = 0
  ev.kernelLaunchNumber = kernelLaunchNumber or 0
  ev.isCompleted = false
  ev.cqNumber = 0
  ev.appendTimestampResult = drvl0.create_ze_kernel_timestamp_result_t()
  ev.gatheredData = false
  return ev
end

function Event:getCsvLine()
  return string.format("%d_%d_%d,%s,%d,%d,%d,%dx%dx%d",
    self.cqNumber,
    self.cmdListNumber,
    self.kernelLaunchNumber,
    self.kernel.name,
    self.kernelStart,
    self.kernelEnd,
    self.kernelEnd - self.kernelStart,
    self.kernel.group_size_x,
    self.kernel.group_size_y,
    self.kernel.group_size_z
  )
end

function CommandList:new(handle, number, isImmediate, isSync)
  local cmdList = {}
  setmetatable(cmdList, {__index = self})
  cmdList.handle = handle
  cmdList.number = number
  cmdList.events = {}
  cmdList.isImmediate = isImmediate or false
  cmdList.isSync = isSync or false
  cmdList.isCompleted = false
  return cmdList
end

function CommandList:addKernelEvent(event)
  table.insert(self.events, event)
end

function CommandQueue:new(handle, number, hFence)
  local cmdQueue = {}
  setmetatable(cmdQueue, {__index = self})
  cmdQueue.handle = handle
  cmdQueue.number = number
  cmdQueue.hFence = hFence or nil
  cmdQueue.cmdLists = {}
  cmdQueue.isCompleted = false
  return cmdQueue
end

function CommandQueue:addCommandList(handle)
  table.insert(self.cmdLists, handle)
end

function Kernel:new(handle, name)
  local kernel = {}
  setmetatable(kernel, {__index = self})
  kernel.handle = handle
  kernel.name = name
  kernel.group_size_x = 0
  kernel.group_size_y = 0
  kernel.group_size_z = 0
  return kernel
end

function Kernel:setWorkSize(x, y, z)
  self.group_size_x = x
  self.group_size_y = y
  self.group_size_z = z
end

-- LUA HELPER FUNCTIONS --

function Log(message)
  print("Lua Log: "..message)
end

function GetObject(table, handle)
  for _, v in ipairs(table) do
    if v.handle == handle then
      return v
    end
  end
  return nil
end

function CreateGitsEventPool(hContext)
  if EVENT_POOL_HANDLE == nil then
    local desc = drvl0.create_ze_event_pool_desc_t()
    local eventPoolPtr = gits.allocBytes(PTR_SIZE)
    desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC
    desc.pNext = gits.nullUdt()
    desc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP
    desc.count = MAX_NUMBER_OF_EVENTS_PER_EVENT_POOL
    local retVal = drvl0.zeEventPoolCreate(hContext, desc, gits.nullUdt(), gits.nullUdt(), eventPoolPtr)
    if retVal ~= ZE_RESULT_SUCCESS then
      error("zeEventPoolCreate API call couldn't create event pool.")
    end
    EVENT_POOL_HANDLE = gits.getUdt(eventPoolPtr, 0)
    drvl0.free(desc)
    gits.freeBytes(eventPoolPtr)
  end
end

function CreateGitsKernelEvent()
  local desc = drvl0.create_ze_event_desc_t()
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC
  desc.pNext = gits.nullUdt()
  desc.index = EVENT_INDEX
  desc.signal = 0
  desc.wait = 0
  EVENT_INDEX = EVENT_INDEX + 1
  if EVENT_INDEX >= MAX_NUMBER_OF_EVENTS_PER_EVENT_POOL then
    error("Too many events in the pool, please raise MAX_NUMBER_OF_EVENTS_PER_EVENT_POOL variable in script.")
  end
  local eventPtr = gits.allocBytes(PTR_SIZE)
  local retVal = drvl0.zeEventCreate(EVENT_POOL_HANDLE, desc, eventPtr)
  drvl0.free(desc)
  if retVal ~= ZE_RESULT_SUCCESS then
    error("zeEventCreate API call couldn't create event.")
  end
  local hEvent = gits.getUdt(eventPtr, 0)
  table.insert(GITS_EVENTS, hEvent)
  gits.freeBytes(eventPtr)
  return hEvent
end

function GetSignalEvent(hSignalEvent)
  if gits.udtToInt(hSignalEvent) ~= 0 then
    return hSignalEvent
  else
    return CreateGitsKernelEvent()
  end
end

function RegisterKernelEvent(hKernel, hEvent, hCommandList)
    APPEND_LAUNCH_KERNEL_NUMBER = APPEND_LAUNCH_KERNEL_NUMBER + 1
    local e = Event:new(hKernel, hEvent, hCommandList, APPEND_LAUNCH_KERNEL_NUMBER)
    table.insert(EVENT_LIST, e)
    local cmdList = GetObject(COMMAND_LIST, hCommandList)
    cmdList:addKernelEvent(e)
    return e
end

function RegisterCmdListKernel(hKernel, signalEvent, hCommandList)
  local e = RegisterKernelEvent(hKernel, signalEvent, hCommandList)
  local cmdList = GetObject(COMMAND_LIST, hCommandList)
  if cmdList.isImmediate and cmdList.isSync then
    local cq = GetObject(COMMAND_QUEUE_LIST, hCommandList)
    UpdateEvent(e, cq.number)
  end
end

function RegisterCmdQueue(hCommandQueue, number, hFence, phCommandLists, numCommandLists)
  local cq = CommandQueue:new(hCommandQueue, number, hFence)
  for i = 0, numCommandLists - 1 do
    local hCommandList = gits.getUdt(phCommandLists, i)
    cq:addCommandList(hCommandList)
  end
  local queueContainsKernel = false
  for _, v in ipairs(cq.cmdLists) do
    local cmdList = GetObject(COMMAND_LIST, v)
    if #cmdList.events > 0 then
      queueContainsKernel = true
      for _, event in ipairs(cmdList.events) do
        event.cqNumber = number
      end
    end
  end
  if queueContainsKernel then
    table.insert(COMMAND_QUEUE_LIST, cq)
  end
end

function AppendQueryEvent(hCommandList, hEvent)
  for _, v in ipairs(EVENT_LIST) do
    if v.handle == hEvent and v.hCommandList == hCommandList and not v.isCompleted and not v.gatheredData then
      local ptr = gits.allocBytes(PTR_SIZE)
      gits.setUdt(ptr, 0, hEvent)
      drvl0.zeCommandListAppendBarrier(hCommandList, gits.nullUdt(), 0, gits.nullUdt())
      drvl0.zeCommandListAppendQueryKernelTimestamps(hCommandList, 1, ptr, v.appendTimestampResult.__self, gits.nullUdt(), gits.nullUdt(), 0, gits.nullUdt())
      drvl0.zeCommandListAppendBarrier(hCommandList, gits.nullUdt(), 0, gits.nullUdt())
      gits.freeBytes(ptr)
      v.gatheredData = true
      return
    end
  end
end

function CalculateKernelStartTime(contextStartTime)
  Log("Kernel Start Time: "..contextStartTime .. " * ".. TIMER_RESOLUTION .. " = ".. contextStartTime * TIMER_RESOLUTION)
  return contextStartTime * TIMER_RESOLUTION
end

function CalculateKernelEndTime(contextStartTime, contextEndTime)
  local endTime = contextEndTime
  Log("> Context Start Time: "..contextStartTime)
  Log("> Context End Time: "..contextEndTime)
  if endTime < contextStartTime then
    local timestampMaxValue = (1 << KERNEL_TIMESTAMP_VALID_BITS) - 1
    endTime = endTime + timestampMaxValue - contextStartTime;
  end
  Log("Kernel End Time: "..endTime .. " * ".. TIMER_RESOLUTION .. " = ".. endTime * TIMER_RESOLUTION)
  return endTime * TIMER_RESOLUTION
end

function UpdateEvent(event, cqNumber)
  if not event.isCompleted then
    Log("Updating event of kernel("..cqNumber.."_"..event.cmdListNumber.."_"..event.kernelLaunchNumber.."): "..event.kernel.name)
    if not event.gatheredData then
      drvl0.zeEventQueryKernelTimestamp(event.handle, event.appendTimestampResult)
    else
      event.appendTimestampResult = drvl0.create_ze_kernel_timestamp_result_t(event.appendTimestampResult.__self)
    end
    local contextStartTime = event.appendTimestampResult.context.kernelStart
    event.kernelStart = CalculateKernelStartTime(contextStartTime)
    local contextEndTime = event.appendTimestampResult.context.kernelEnd
    event.kernelEnd = CalculateKernelEndTime(contextStartTime, contextEndTime)
    drvl0.free(event.appendTimestampResult)
    event.isCompleted = true
    event.cqNumber = cqNumber
  end
end

function UpdateEvents(cmdListHandle, kernelHandle)
  for _, v in ipairs(EVENT_LIST) do
    if (v.kernel.handle == kernelHandle and v.hCommandList == cmdListHandle) then
      UpdateEvent(v, v.cqNumber)
    end
  end
end

function UpdateEventsFromCommandList(cmdListHandle)
  for _, v in ipairs(COMMAND_LIST) do
    if not v.isCompleted and v.handle == cmdListHandle then
      for _, kernelEvent in ipairs(v.events) do
        UpdateEvents(cmdListHandle, kernelEvent.kernel.handle)
      end
      v.isCompleted = true
    end
  end
end

function UpdateEventsFromFence(hFence)
  for _, cqState in ipairs(COMMAND_QUEUE_LIST) do
    if cqState.hFence == hFence and not cqState.isCompleted then
      for _, cmdListHandle in ipairs(cqState.cmdLists) do
        UpdateEventsFromCommandList(cmdListHandle)
      end
      cqState.isCompleted = true
    end
  end
end

function UpdateEventsFromCommandQueue(hCommandQueue)
  for _, cqState in ipairs(COMMAND_QUEUE_LIST) do
    if not cqState.isCompleted and cqState.handle == hCommandQueue then
      for _, cmdListState in ipairs(cqState.cmdLists) do
        UpdateEventsFromCommandList(cmdListState)
      end
      cqState.isCompleted = true
    end
  end
end

function ObtainTimerResolution(hDevice)
  local properties = drvl0.create_ze_device_properties_t()
  properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES
  properties.pNext = gits.nullUdt()
  local propRetVal = drvl0.zeDeviceGetProperties(hDevice, properties)
  if propRetVal == ZE_RESULT_SUCCESS and properties.type == ZE_DEVICE_TYPE_GPU then
    TIMER_RESOLUTION = properties.timerResolution
    KERNEL_TIMESTAMP_VALID_BITS = properties.kernelTimestampValidBits
    Log("Timer Resolution: "..TIMER_RESOLUTION)
    Log("Kernel Timestamp Valid Bits: "..KERNEL_TIMESTAMP_VALID_BITS)
  end
  drvl0.free(properties)
end

function DeepCopy(src)
  local type_ = type(src)
  if type_ == 'table' then
    local dst = {}
    for srcKey, srcVal in pairs(src) do
      dst[ DeepCopy(srcKey) ] = DeepCopy(srcVal)
    end
    src_meta = getmetatable(src)
    setmetatable(dst, DeepCopy(getmetatable(src)))
    return dst
  else
    local dst = src
    return dst
  end
end

function ResetEventsInCmdList(hCommandQueue, cmdList, cqNumber)
  for _, v in ipairs(cmdList.events) do
    for _, ev in ipairs(EVENT_LIST) do
      if v == ev.hKernel and v.hCommandList == cmdList and v.isCompleted then
        drvl0.zeEventHostReset(ev.handle)
        local e = DeepCopy(ev)
        e.isCompleted = false
        e.hCommandQueue = hCommandQueue
        e.cqNumber = cqNumber
        table.insert(EVENT_LIST, e)
      end
    end
  end
end

function CheckForRepeatedCommandLists(hCommandQueue, numCommandLists, phCommandLists, cqNumber)
  -- Case when command list is already queued for execution.
  for _, v in ipairs(COMMAND_QUEUE_LIST) do
    for i = 0, numCommandLists - 1 do
      local cmdListHandle = gits.getUdt(phCommandLists, i)
      for _, vv in ipairs(v.cmdLists) do
        if vv == cmdListHandle then
          local cmdList = GetObject(COMMAND_LIST, cmdListHandle)
          if v.handle ~= hCommandQueue and not cmdList.isCompleted and not v.isCompleted and #cmdList.events > 0 then
            zeCommandQueueSynchronize(v.handle, UINT64_MAX)
          end
          if cmdList.isCompleted then
            ResetEventsInCmdList(hCommandQueue, cmdList, cqNumber)
            cmdList.isCompleted = false
          end
        end
      end
    end
  end
end

function CompareGitsNomenclature(a, b)
  if a.cqNumber == b.cqNumber then
    if a.cmdListNumber == b.cmdListNumber then
      return a.kernelLaunchNumber < b.kernelLaunchNumber
    end
    return a.cmdListNumber < b.cmdListNumber
  end
  return a.cqNumber < b.cqNumber
end

function gitsProgramExit()
    local csvFile = io.open('benchmark.csv', 'w')
    if csvFile == nil then
      error('Failed to open results file for writing.')
      return
    end

    io.output(csvFile)

    local header = ''
    for index, col_name in ipairs(CSV_HEADER) do
      header = header .. col_name
      if index ~= #CSV_HEADER then
        header = header .. ','
      end
    end
    io.write(header .. '\n')

    table.sort(EVENT_LIST, CompareGitsNomenclature)
    for _, event in ipairs(EVENT_LIST) do
      io.write(event:getCsvLine() .. '\n')
    end

    io.close(csvFile)
    for _, v in ipairs(GITS_EVENTS) do
      drvl0.zeEventDestroy(v)
    end
    if EVENT_POOL_HANDLE ~= nil then
      drvl0.zeEventPoolDestroy(EVENT_POOL_HANDLE)
    end
end

-- API CALLS --

function zeKernelCreate(hModule, desc, phKernel)
  local retVal = drvl0.zeKernelCreate(hModule, desc, phKernel)
  if retVal == ZE_RESULT_SUCCESS then
    local kernel = Kernel:new(gits.getUdt(phKernel, 0), gits.udtToStr(desc.pKernelName))
    table.insert(KERNEL_LIST, kernel)
  end
  return retVal
end

function zeDeviceGet(hDriver, pCount, phDevices)
  local retVal = drvl0.zeDeviceGet(hDriver, pCount, phDevices)
  if TIMER_RESOLUTION == 0 and phDevices ~= gits.nullUdt() and retVal == ZE_RESULT_SUCCESS then
    for i = 0, gits.getInt(pCount, 0) - 1 do
      if TIMER_RESOLUTION == 0 then
        ObtainTimerResolution(gits.getUdt(phDevices, i))
      end
    end
  end
  return retVal
end

function zeDeviceGetProperties(hDevice, pDeviceProperties)
  local retVal = drvl0.zeDeviceGetProperties(hDevice, pDeviceProperties)
  if TIMER_RESOLUTION == 0 and retVal == ZE_RESULT_SUCCESS and pDeviceProperties.type == ZE_DEVICE_TYPE_GPU then
    TIMER_RESOLUTION = pDeviceProperties.timerResolution
    KERNEL_TIMESTAMP_VALID_BITS = pDeviceProperties.kernelTimestampValidBits
    Log("Timer Resolution: "..TIMER_RESOLUTION)
    Log("Kernel Timestamp Valid Bits: "..KERNEL_TIMESTAMP_VALID_BITS)
  end
  return retVal
end

function zeCommandListCreate(hContext, hDevice, desc, phCommandList)
  CreateGitsEventPool(hContext)
  local retVal = drvl0.zeCommandListCreate(hContext, hDevice, desc, phCommandList)
  if retVal == ZE_RESULT_SUCCESS then
    local hCommandList = gits.getUdt(phCommandList, 0)
    COMMAND_LIST_NUMBER = COMMAND_LIST_NUMBER + 1
    local cmdList = CommandList:new(hCommandList, COMMAND_LIST_NUMBER)
    table.insert(COMMAND_LIST, cmdList)
    if TIMER_RESOLUTION == 0 then
      ObtainTimerResolution(hDevice)
    end
  end
  return retVal
end

function zeCommandListCreateImmediate(hContext, hDevice, altdesc, phCommandList)
  CreateGitsEventPool(hContext)
  local retVal = drvl0.zeCommandListCreateImmediate(hContext, hDevice, altdesc, phCommandList)
  if retVal == ZE_RESULT_SUCCESS then
    local hCommandList = gits.getUdt(phCommandList, 0)
    COMMAND_LIST_NUMBER = COMMAND_LIST_NUMBER + 1
    SUBMIT_QUEUE_NUMBER = SUBMIT_QUEUE_NUMBER + 1
    local isSync = altdesc.mode == ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS -- default is ASYNC
    local cmdList = CommandList:new(hCommandList, COMMAND_LIST_NUMBER, true, isSync)
    table.insert(COMMAND_LIST, cmdList)
    local cmdQueue = CommandQueue:new(hCommandList, SUBMIT_QUEUE_NUMBER)
    table.insert(COMMAND_QUEUE_LIST, cmdQueue)
    if TIMER_RESOLUTION == 0 then
      ObtainTimerResolution(hDevice)
    end
  end
  return retVal
end

function zeKernelSetGroupSize(hKernel, groupSizeX, groupSizeY, groupSizeZ)
  local retVal = drvl0.zeKernelSetGroupSize(hKernel, groupSizeX, groupSizeY, groupSizeZ)
  if retVal == ZE_RESULT_SUCCESS then
    local kernel = GetObject(KERNEL_LIST, hKernel)
    kernel:setWorkSize(groupSizeX, groupSizeY, groupSizeZ)
  end
  return retVal
end

function zeCommandListAppendLaunchCooperativeKernel(hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, numWaitEvents, phWaitEvents)
  local signalEvent = GetSignalEvent(hSignalEvent)
  local retVal = drvl0.zeCommandListAppendLaunchCooperativeKernel(hCommandList, hKernel, pLaunchFuncArgs, signalEvent, numWaitEvents, phWaitEvents)
  if retVal == ZE_RESULT_SUCCESS then
    RegisterCmdListKernel(hKernel, signalEvent, hCommandList)
  end
  return retVal
end

function zeCommandListAppendLaunchKernel(hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, numWaitEvents, phWaitEvents)
  local signalEvent = GetSignalEvent(hSignalEvent)
  local retVal = drvl0.zeCommandListAppendLaunchKernel(hCommandList, hKernel, pLaunchFuncArgs, signalEvent, numWaitEvents, phWaitEvents)
  if retVal == ZE_RESULT_SUCCESS then
    RegisterCmdListKernel(hKernel, signalEvent, hCommandList)
  end
  return retVal
end

function zeCommandListAppendLaunchKernelIndirect(hCommandList, hKernel, pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents, phWaitEvents)
  local signalEvent = GetSignalEvent(hSignalEvent)
  local retVal = drvl0.zeCommandListAppendLaunchKernelIndirect(hCommandList, hKernel, pLaunchArgumentsBuffer, signalEvent, numWaitEvents, phWaitEvents)
  if retVal == ZE_RESULT_SUCCESS then
    RegisterCmdListKernel(hKernel, signalEvent, hCommandList)
  end
  return retVal
end

function zeCommandListAppendLaunchMultipleKernelsIndirect(hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents, phWaitEvents)
  local signalEvent = GetSignalEvent(hSignalEvent)
  local retVal = drvl0.zeCommandListAppendLaunchMultipleKernelsIndirect(hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer, signalEvent, numWaitEvents, phWaitEvents)
  if retVal == ZE_RESULT_SUCCESS then
    for i = 0, numKernels - 1 do
      local hKernel = gits.getUdt(phKernels, i)
      RegisterCmdListKernel(hKernel, signalEvent, hCommandList)
    end
  end
  return retVal
end

function zeCommandQueueExecuteCommandLists(hCommandQueue, numCommandLists, phCommandLists, hFence)
  SUBMIT_QUEUE_NUMBER = SUBMIT_QUEUE_NUMBER + 1
  CheckForRepeatedCommandLists(hCommandQueue, numCommandLists, phCommandLists, SUBMIT_QUEUE_NUMBER)
  local retVal = drvl0.zeCommandQueueExecuteCommandLists(hCommandQueue, numCommandLists, phCommandLists, hFence)
  RegisterCmdQueue(hCommandQueue, SUBMIT_QUEUE_NUMBER, hFence, phCommandLists, numCommandLists)
  return retVal
end

function zeFenceReset(hFence)
  local retVal = drvl0.zeFenceReset(hFence)
  if retVal == ZE_RESULT_SUCCESS then
    for _, cqState in ipairs(COMMAND_QUEUE_LIST) do
      if cqState.hFence == hFence then
        cqState.hFence = nil
      end
    end
  end
  return retVal
end

function zeCommandListReset(hCommandList)
  local cmdList = GetObject(COMMAND_LIST, hCommandList)
  cmdList.isCompleted = false
  cmdList.events = {}
  return drvl0.zeCommandListReset(hCommandList)
end

function zeCommandQueueSynchronize(hCommandQueue, timeout)
  local retVal = drvl0.zeCommandQueueSynchronize(hCommandQueue, timeout)
  if retVal == ZE_RESULT_SUCCESS then
    UpdateEventsFromCommandQueue(hCommandQueue)
  end
  return retVal
end

function zeCommandListHostSynchronize(hCommandList, timeout)
  local retVal = drvl0.zeCommandListHostSynchronize(hCommandList, timeout)
  if retVal == ZE_RESULT_SUCCESS then
    UpdateEventsFromCommandList(hCommandList)
  end
  return retVal
end

function zeEventHostSynchronize(hEvent, timeout)
  local retVal = drvl0.zeEventHostSynchronize(hEvent, timeout)
  local event = GetObject(EVENT_LIST, hEvent)
  if event ~= nil and retVal == ZE_RESULT_SUCCESS then
    -- Immediate async command list case.
    local cq = GetObject(COMMAND_QUEUE_LIST, event.hCommandList)
    if cq ~= nil then
      UpdateEvent(event, cq.number)
      return retVal
    end
    -- Command queue execution synchronization case.
    for _, e in ipairs(EVENT_LIST) do
      if e.handle == hEvent and not e.isCompleted then
        for _, cq in ipairs(COMMAND_QUEUE_LIST) do
          for _, hCommandList in ipairs(cq.cmdLists) do
            local cmdList = GetObject(COMMAND_LIST, hCommandList)
            for _, kernelEvent in ipairs(cmdList.events) do
              if e.kernel.handle == kernelEvent.kernel.handle and e.hCommandList == hCommandList then
                UpdateEvent(e, cq.number)
              end
            end
          end
        end
      end
    end
  end
  return retVal
end

function zeFenceHostSynchronize(hFence, timeout)
  local retVal = drvl0.zeFenceHostSynchronize(hFence, timeout)
  if retVal == ZE_RESULT_SUCCESS then
    UpdateEventsFromFence(hFence)
  end
  return retVal
end

function zeFenceQueryStatus(hFence)
  local retVal = drvl0.zeFenceQueryStatus(hFence)
  if retVal == ZE_RESULT_SUCCESS then
    UpdateEventsFromFence(hFence)
  end
  return retVal
end

function zeCommandListAppendEventReset(hCommandList, hEvent)
  AppendQueryEvent(hCommandList, hEvent)
  return drvl0.zeCommandListAppendEventReset(hCommandList, hEvent)
end

function zeEventPoolCreate(hContext, desc, numDevices, phDevices, phEventPool)
  if desc.flags & ZE_EVENT_POOL_FLAG_IPC then
    desc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP
  else
    desc.flags = desc.flags | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP
  end
  return drvl0.zeEventPoolCreate(hContext, desc, numDevices, phDevices, phEventPool)
end

function zeEventDestroy(hEvent)
  -- Check for not explicitly finished events.
  for _, event in ipairs(EVENT_LIST) do
    if event.handle == hEvent and not event.isCompleted then
      local eventFinished = drvl0.zeEventQueryStatus(event.handle)
      if eventFinished ~= ZE_RESULT_SUCCESS then
        drvl0.zeEventHostSynchronize(event.handle, UINT64_MAX)
      end
      UpdateEvent(event, event.cqNumber)
    end
  end
  return drvl0.zeEventDestroy(hEvent)
end
