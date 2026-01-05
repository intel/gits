-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2026 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

---------------------------------------------------------------------------------------------------
-- Description
---------------------------------------------------------------------------------------------------
-- The goal of this script is to measure the GPU performance at a per-draw, per-tuple (i.e., shader
-- change), or per-frame level in a vendor-agnostic fashion through the use of query objects. It 
-- does so by injecting extra measuring API calls in the stream runtime and processes and writes
-- the results to a csv file.
--
-- This script has been updated to collect and output GPU times that are aligned with how they are
-- reported with tools like Giraffe. It also includes other relevant metadata associated with each
-- sample, such as the frame number, program number, draw type, draw mode, element count, and more.
---------------------------------------------------------------------------------------------------

---------------------------------------------------------------------------------------------------
-- Enums
---------------------------------------------------------------------------------------------------
-- GL Defines
GL_TIME_ELAPSED   = 0x88BF
GL_QUERY_RESULT   = 0x8866
GL_POINTS         = 0x0000
GL_LINES          = 0x0001
GL_LINE_LOOP      = 0x0002
GL_LINE_STRIP     = 0x0003
GL_TRIANGLES      = 0x0004
GL_TRIANGLE_STRIP = 0x0005
GL_TRIANGLE_FAN   = 0x0006
GL_QUADS          = 0x0007
GL_QUAD_STRIP     = 0x0008
GL_POLYGON        = 0x0009
GL_PATCHES        = 0x000E
GL_SAMPLES        = 0x80A9

-- API
WGL_API = 1
GLX_API = 2
EGL_API = 3

-- Modes
OFF = -1
PER_DRAW = 0
PER_TUPLE = 1
PER_FRAME = 2

---------------------------------------------------------------------------------------------------
-- Options (these are for you to edit)
---------------------------------------------------------------------------------------------------
FRAME_TO_QUERY = 0   -- Frame to query, or zero to query all.
MODE = PER_FRAME
COLLECT_RESULTS_PER_TIMER_STOP = true -- TODO: seems to help with run-to-run variance.
PRINT_ON = false -- Prints debug info to stdout (frame number, context creation, etc.)
QUERY_MSAA_COUNT_PER_DRAW = false -- Query and output MSAA count per draw in per-draw mode

---------------------------------------------------------------------------------------------------
-- Globals
---------------------------------------------------------------------------------------------------
initialized = false
nativeAPI = 0
useEXTQueries = false
currCtx = nil
currDrawSurf = nil
currReadSurf = nil
lastCtx = nil
lastDrawSurf = nil
lastReadSurf = nil
display = nil
file = nil
frameNo = 1
drawNo = 0
ptbrSamplingMode = -1
programNo = 0
programNoUsed = -1
drawType = ""
elementCount = 0
draw_mode = ""
perTupleIsRecording = false
triggerId = 0
msaa_count = 1

apiset = "<default>"
filename = "<unset>"

---------------------------------------------------------------------------------------------------
-- API calls cross platform dispatcher
---------------------------------------------------------------------------------------------------
function genQueries(n, q)
  if useEXTQueries then
    drv.glGenQueriesEXT(n, q)
  else
    drv.glGenQueries(n, q)
  end
end

function beginQuery(t, i)
  if useEXTQueries then
    drv.glBeginQueryEXT(t, i)
  else
    drv.glBeginQuery(t, i)
  end
end

function endQuery(t)
  if useEXTQueries then
    drv.glEndQueryEXT(t)
  else
    drv.glEndQuery(t)
  end
end

function getQueryObjectuiv(i, p, r)
  if useEXTQueries then
    drv.glGetQueryObjectuivEXT(i, p, r)
  else
    drv.glGetQueryObjectuiv(i, p, r)
  end
end

function getProcAddress(name)
  if nativeAPI == WGL_API then
    return drv.wglGetProcAddress(name)
  end
  if nativeAPI == EGL_API then
    return drv.eglGetProcAddress(name)
  end
  if nativeAPI == GLX_API then
    return drv.glXGetProcAddress(name)
  end
  error("Unknown native API")
end

function setCurrentContext(ctx, draw, read)
  trackcontext(ctx, draw, read)
  if nativeAPI == WGL_API then
    return drv.wglMakeCurrent(draw, ctx)
  end
  if nativeAPI == EGL_API then
    return drv.eglMakeCurrent(display, draw, read, ctx)
  end
  if nativeAPI == GLX_API then
    return drv.glXMakeContextCurrent(display, draw, read, ctx)
  end
end

---------------------------------------------------------------------------------------------------
-- Tools
---------------------------------------------------------------------------------------------------

--Vector
Vector = {counter = 0, arr={}}

function Vector:new(o)
  local new_vec = o or {}
  setmetatable(new_vec, self)
  self.__index = self
  return new_vec
end

function Vector:__newindex(t, k, v)
  error("Vector error: Not supported operation.")
end

function Vector:push_back(o)
  self.arr[self.counter] = o
  self.counter = self.counter + 1
end

function Vector:size()
  return self.counter
end

function Vector:array()
  return self.arr
end

function Vector:back()
  return self.arr[self.counter-1]
end

function Vector:clear()
  for i=0, self.counter-1, 1 do
    self.arr[i] = nil
  end
  self.counter=0
end


--Simple funcs
function deepcopy(src)
  local type_ = type(src)
  if type_ == 'table' then
    local dst = {}
    for srcKey, srcVal in pairs(src) do
      dst[ deepcopy(srcKey) ] = deepcopy(srcVal)
    end
    src_meta = getmetatable(src)
    if src_meta ~= nil and src_meta.__index == Vector then --detect protocol
      setmetatable(dst, getmetatable(src))
    else
      setmetatable(dst, deepcopy(getmetatable(src)))
    end
    return dst
  else
    local dst = src
    return dst
  end
end

function sizeof(arr)
  if arr == nil then
    return 0
  end

  local len=0
  for k,v in pairs(arr) do
    len = len + 1
  end
  return len
end

function findinlist(item, list)
  if list == nil then
    return nil
  end

  for key, val in pairs(list) do
    if key == item then
      return key
    end
  end
  return nil
end

function cleanlist(list)
  for k,v in pairs(list) do
    list[k]=nil
  end
end

function modeString(mode)
  if (mode == GL_POINTS) then
    return "GL_POINTS"
  elseif(mode == GL_LINES) then
    return "GL_LINES"
  elseif(mode == GL_LINE_LOOP) then
    return "GL_LINE_LOOP"
  elseif(mode == GL_LINE_STRIP) then
    return "GL_LINE_STRIP"
  elseif(mode == GL_TRIANGLES) then
    return "GL_TRIANGLES"
  elseif(mode == GL_TRIANGLE_STRIP) then
    return "GL_TRIANGLE_STRIP"
  elseif(mode == GL_TRIANGLE_FAN) then
    return "GL_TRIANGLE_FAN"
  elseif(mode == GL_QUADS) then
    return "GL_QUADS"
  elseif(mode == GL_QUAD_STRIP) then
    return "GL_QUAD_STRIP"
  elseif(mode == GL_POLYGON) then
    return "GL_POLYGON"
  elseif(mode == GL_PATCHES) then
    return "GL_PATCHES"
  else
    return "UNKNOWN"
  end
end

function printMsg(msg)
  if (PRINT_ON) then
    print(msg)
  end
end

---------------------------------------------------------------------------------------------------
-- Init
---------------------------------------------------------------------------------------------------
function init(ntvApi)
  if initialized == true then
    return
  end

  if (MODE == OFF) then
    return
  end

  nativeAPI = ntvApi

  --Open file
  local csv_name = ""
  if MODE == PER_DRAW then
    csv_name = "benchmarkGPUPerDraw.csv"
  elseif MODE == PER_TUPLE then
    csv_name = "benchmarkGPUPerTuple.csv"
  elseif MODE == PER_FRAME then 
    csv_name = "benchmarkGPUPerFrame.csv"
  else 
    csv_name = "benchmarkGPU.csv"
  end
  file = io.open(csv_name, "w")
  if file == nil then
    error("Can't open results file for writing.")
  end

  --Choose query objects type
  local coreNamePtr = gits.allocUdtFromStr("glBeginQuery")
  local coreApiPtr = getProcAddress(coreNamePtr)
  if (coreApiPtr ~= gits.nullUdt()) then
    useEXTQueries = false
  else
    local extNamePtr = gits.allocUdtFromStr("glBeginQueryEXT")
    local extApiPtr = getProcAddress(extNamePtr)
    if (extApiPtr ~= gits.nullUdt()) then
      useEXTQueries = true
    else
      error("Query objects not supported on current context.")
    end
    gits.freeBytes(extNamePtr)
  end
  gits.freeBytes(coreNamePtr)

  initialized = true
end

---------------------------------------------------------------------------------------------------
-- GPU Timer
---------------------------------------------------------------------------------------------------
Result = { timer=0, startDraw=0, endDraw=0, time=0, frame=0}
Query = { objPtr=0, finished=false, result=nil }
Context = { queries = Vector:new{counter=0, arr={}}, currDrawNr = 0 }
results = {}
contexts = {}
currTimerNr = 0
gpuTimerEnabled = true

-- GPU Timer Start
function GPUTimerStart()
  if gpuTimerEnabled == false then
    return
  end

  local ctxData = contexts[currCtx]
  local queries = ctxData.queries

  --Check if previous timer finished
  if queries:size()>0 and queries:back().finished == false then
    error("GPU Timer started before previous one has been finished.")
  end

  --Add new entry in tracked data
  queries:push_back(deepcopy(Query))
  queries:back().result = deepcopy(Result)
  queries:back().result.timer = currTimerNr
  queries:back().result.startDraw = ctxData.currDrawNr
  queries:back().result.frame = frameNo
  queries:back().result.programNo = string.format("%d", programNo)
  queries:back().result.drawType = drawType
  queries:back().result.draw_mode = draw_mode
  queries:back().result.elementCount = string.format("%d", elementCount)
  queries:back().result.triggerId = triggerId
  queries:back().result.msaaCount = msaa_count

  currTimerNr = currTimerNr + 1

  --Create and start timer
  queries:back().objPtr = gits.allocBytes(8)
  genQueries(1, queries:back().objPtr)
  beginQuery(GL_TIME_ELAPSED, gits.getInt(queries:back().objPtr, 0))
end

-- GPU Timer Stop
function GPUTimerStop()
  if gpuTimerEnabled == false then
    return
  end

  local ctxData = contexts[currCtx]
  local queries = ctxData.queries

  --Check if last timer is not finished
  if queries:size()==0 or queries:back().finished == true then
    return
  end

  --Stop timer
  endQuery(GL_TIME_ELAPSED)
  queries:back().finished = true
  queries:back().result.endDraw = ctxData.currDrawNr
end

function GPUTimerStopAllContexts()
  for ctx, ctxData in pairs(contexts) do
    local queries = ctxData.queries
    if queries:size() > 0 then
      local ctxChanged = false
      if (ctx ~= currCtx) then
        setCurrentContext(ctx, currDrawSurf, currReadSurf)
        ctxChanged = true
        print("stopped context")
      end

      if queries:back().finished == false then
        --Stop timer
        endQuery(GL_TIME_ELAPSED)
        queries:back().finished = true
        queries:back().result.endDraw = ctxData.currDrawNr
      end

      if ctxChanged == true then
        setCurrentContext(lastCtx, lastDrawSurf, lastReadSurf)
      end
    end
  end
end

-- GPU Collect results
function GPUTimerCollectResults()
  if gpuTimerEnabled == false then
    return
  end

  local ctxData = contexts[currCtx]
  local queries = ctxData.queries
  --Collect timers data
  local timePtr = gits.allocBytes(8)
  local timers={}
  queries_arr = queries:array()
  for i=0, queries:size()-1, 1 do
    if queries_arr[i].finished == true then
      getQueryObjectuiv(gits.getInt(queries_arr[i].objPtr, 0), GL_QUERY_RESULT, timePtr)

      --Store results in results array
      queries_arr[i].result.time = gits.getInt(timePtr, 0)
      results[ sizeof(results) ] = deepcopy(queries_arr[i].result)

      drv.glDeleteQueries(1, queries_arr[i].objPtr)
      gits.freeBytes(queries_arr[i].objPtr)
    else
      error("Not terminated timer found")
    end
  end

  queries:clear()
  gits.freeBytes(timePtr)
end

-- GPU Collect results for all contexts
function GPUTimerCollectAllContextsResults()
  for ctx, ctxData in pairs(contexts) do
    local queries = ctxData.queries
    if queries:size() > 0 then
      local ctxChanged = false
      if (ctx ~= currCtx) then
        setCurrentContext(ctx, currDrawSurf, currReadSurf)
        ctxChanged = true
      end

      if queries:back().finished == false then
        GPUTimerStop()
      end
      GPUTimerCollectResults()

      if ctxChanged == true then
        setCurrentContext(lastCtx, lastDrawSurf, lastReadSurf)
      end
    end
  end
end

function GPUTimerPrintResults()
  io.output(file)
  if (MODE == PER_DRAW) then
    local header = "Frame,Draw,Program,Draw Type,Draw Mode,Element Count"
    if (QUERY_MSAA_COUNT_PER_DRAW == true) then
      header = header .. ",MSAA Count"
    end
    header = header .. ",GPU Time (ms)\n"
    io.write(header)
    
    for i=0,sizeof(results)-1, 1 do
      local row = results[i].frame .. "," .. results[i].timer .. ",".. results[i].programNo .. "," .. results[i].drawType .. "," .. results[i].draw_mode .. "," .. results[i].elementCount
      if (QUERY_MSAA_COUNT_PER_DRAW == true) then
        row = row .. "," .. results[i].msaaCount
      end
      row = row .. "," .. (results[i].time/1000000) .. "\n"
      io.write(row)
    end
  elseif (MODE == PER_TUPLE) then
    io.write("Frame,Program,Trigger Id,GPU Time (ms)\n")
    for i=0,sizeof(results)-1, 1 do
      io.write(results[i].frame .. ",".. results[i].programNo .. "," .. results[i].triggerId .. "," .. (results[i].time/1000000) .. "\n")
    end
  elseif (MODE == PER_FRAME) then
    io.write("Frame,GPU Time (ms)\n")
    for i=0,sizeof(results)-1, 1 do
      io.write(results[i].frame .. "," .. (results[i].time/1000000) .. "\n")
    end
  end
end

---------------------------------------------------------------------------------------------------
-- Actions
---------------------------------------------------------------------------------------------------

-- FBO Bind action
function fboBindAction()
end

my_counter = 0
function preProgramAction(prog_id)
end

function postProgramAction(prog_id)
  programNo = prog_id
end

-- Draw actions
local function preDrawActionImpl(funcName, mode, count)
   -- Benchmark GPU 
  if (FRAME_TO_QUERY ~= 0 and frameNo ~= FRAME_TO_QUERY) then
    return
  end
  
  if (MODE == PER_TUPLE) then
    if (programNo ~= programNoUsed) then
      if (perTupleIsRecording) then
        stopTimerAllContexts()
      end
      startTimer()
      perTupleIsRecording = true
    end
  elseif (MODE == PER_DRAW) then
    if (QUERY_MSAA_COUNT_PER_DRAW == true) then
      local samples = gits.allocBytes(4)
      drv.glGetIntegerv(GL_SAMPLES, samples)
      msaa_count = gits.getInt(samples, 0)
      gits.freeBytes(samples)
    end
    drawType = funcName
    elementCount = count
    draw_mode = modeString(mode)
    startTimer()
  end
end

function preDrawAction(funcName, mode, count)
  -- Handle optional parameters
  _mode = ""
  _count = 0

  if (mode ~= nil) then
    _mode = mode
  end

  if (count ~= nil) then
    _count = count
  end

  preDrawActionImpl(funcName, _mode, _count)
end

function postDrawAction()
  -- Benchmark GPU
  if (FRAME_TO_QUERY ~= 0 and frameNo ~= FRAME_TO_QUERY) then
    return
  end

  if (MODE == PER_DRAW) then
    stopTimer()
  end

  programNoUsed = programNo
  drawNo = drawNo + 1
end

-- End frame actions
function preEndFrameAction()
  if (MODE == PER_FRAME) and (frameNo ~= 1) then 
    stopTimerAllContexts()
  end

  if ((MODE == PER_TUPLE) and perTupleIsRecording) then
    stopTimerAllContexts()
    perTupleIsRecording = false
  end

  if (FRAME_TO_QUERY == 0) or (FRAME_TO_QUERY == frameNo) then
    GPUTimerCollectAllContextsResults()
  end

  frameNo = frameNo + 1
  drawNo = 0
end

function postEndFrameAction()
    if (MODE == PER_FRAME) then
      startTimer()
    end
end

function startTimer()
  local ctxData = contexts[currCtx]
  contexts[currCtx].currDrawNr = contexts[currCtx].currDrawNr + 1

  if gpuTimerEnabled == true then
    GPUTimerStart()
  end
end

function stopTimer()
  if gpuTimerEnabled == true then
    GPUTimerStop()
    if (COLLECT_RESULTS_PER_TIMER_STOP == true) then
      GPUTimerCollectResults()
    end
    triggerId = triggerId + 1
  end
end

function stopTimerAllContexts()
  if gpuTimerEnabled == true then
    if (COLLECT_RESULTS_PER_TIMER_STOP == true) then
      -- This also stops the timer
      GPUTimerCollectAllContextsResults()
    else
      GPUTimerStopAllContexts()
    end
    triggerId = triggerId + 1
  end
end

-- start action
function startAction()
  args = gits.getArgsStr()

  items = {}
  count = 0
  for x in args:gmatch('%S+') do
    items[count] = x
    count = count + 1
  end

  print (items)

  if count > 2 then
    metricSet = items[0]
    filename = items[1]
    if items[2] == "PtbrPerDrawEnabled" then
      ptbrSamplingMode = 0
    elseif items[2] == "PtbrPerTileEnabled" then
      ptbrSamplingMode = 1
    end
  elseif count == 2 then
    metricSet = items[0]
    filename = items[1]
  elseif count == 1 then
    metricSet = items[0]
    filename = 'metrics_blah_results.csv'
  end
end

-- Exit action
function exitAction()
    if (mode == PER_FRAME) then
      stopTimer()
    end

    if (mode ~= OFF) then
      GPUTimerPrintResults()
    end

    if (file ~= nil) then
      io.close(file)
    end
end

function trackcontext(ctx, draw, read)
  lastCtx = currCtx
  lastDrawSurf = currDrawSurf
  lastReadSurf = currReadSurf
  currCtx = ctx
  currDrawSurf = draw
  currReadSurf = read
  if contexts[currCtx] == nil then
    contexts[currCtx] = deepcopy(Context)
  end
end

---------------------------------------------------------------------------------------------------
-- GITS LUA functions
---------------------------------------------------------------------------------------------------
function gitsFrameBegin(no)
  if (PRINT_ON) then
    printMsg('FrameStart: ' .. no)
  end
end

function gitsFrameEnd(no)

end

function gitsProgramStart()
  startAction()
end

function gitsProgramExit()
  exitAction()
end

function gitsStateRestoreBegin()
  gpuTimerEnabled = false
end

function gitsStateRestoreEnd()
  gpuTimerEnabled = true
end

---------------------------------------------------------------------------------------------------
-- API Interception
---------------------------------------------------------------------------------------------------
function wglCreateContext(hdc)
  printMsg("Context Creation")
  return drv.wglCreateContext(hdc)
end

function wglCreateContextAttribsARB(hdc, share, attribs)
  return drv.wglCreateContextAttribsARB(hdc, share, attribs)
end

function eglCreateContext(disp, config, share, attribs)
  return drv.eglCreateContext(disp, config, share, attribs)
end

function glXCreateContext(dpy, vis, shareList, direct)
  return drv.glXCreateContext(dpy, vis, shareList, direct)
end

function glXCreateContextAttribsARB(dpy, config, shareList, direct, attribs)
  return drv.glXCreateContextAttribsARB(dpy, config, shareList, direct, attribs)
end

-- Ensure all queries are read before context is destroyed.
function wglDeleteContext(hglrc)
  return drv.wglDeleteContext(hglrc)
end

function eglDestroyContext(disp, ctx)
  return drv.eglDestroyContext(disp, ctx)
end

function glDeleteFramebuffers(num, framebuffers)
  return drv.glDeleteFramebuffers(num, framebuffers)
end

function wglMakeCurrent(hdc, hglrc)
  local retVal = drv.wglMakeCurrent(hdc, hglrc)
  init(WGL_API)
  trackcontext(hglrc, hdc, hdc)
  return retVal
end

function glXMakeCurrent(dpy, drawable, ctx)
  local retVal = drv.glXMakeCurrent(dpy, drawable, ctx)
  init(GLX_API)
  trackcontext(ctx, drawable, drawable)
  return retVal
end

function glXMakeContextCurrent(dpy, draw, read, ctx)
  local retVal = drv.glXMakeContextCurrent(dpy, draw, read, ctx)
  init(GLX_API)
  trackcontext(ctx, draw, read)
  return retVal
end

function eglMakeCurrent(dpy, draw, read, context)
  init(EGL_API)
  trackcontext(context, draw, read)
  return drv.eglMakeCurrent(dpy, draw, read, context)
end

function glBindFramebuffer(target, name)
  drv.glBindFramebuffer(target, name)
  fboBindAction()
end

function glBindFramebufferEXT(target, name)
  drv.glBindFramebufferEXT(target, name)
  fboBindAction()
end

function wglSwapBuffers(hdc)
  preEndFrameAction()
  drv.wglSwapBuffers(hdc)
  postEndFrameAction()
end

function glXSwapBuffers(dpy, surf)
  preEndFrameAction()
  drv.glXSwapBuffers(dpy, surf)
  postEndFrameAction()
end

function eglSwapBuffers(dpy, surf)
  preEndFrameAction()
  drv.eglSwapBuffers(dpy, surf)
  postEndFrameAction()
end

-- Drawcalls
function glBegin(mode)
  preDrawAction("glBegin", mode, 0)
  drv.glBegin(mode)
end

function glEnd()
  drv.glEnd()
  postDrawAction()
end

function glArrayElement(i)
  preDrawAction("glArrayElement")
  drv.glArrayElement(i)
  postDrawAction()
end
function glArrayElementEXT(i)
  preDrawAction("glArrayElementEXT")
  drv.glArrayElementEXT(i)
  postDrawAction()
end
function glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  preDrawAction("glBlitFramebuffer")
  drv.glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  postDrawAction()
end
function glBlitFramebufferANGLE(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  preDrawAction("glBlitFramebufferANGLE")
  drv.glBlitFramebufferANGLE(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  postDrawAction()
end
function glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  preDrawAction("glBlitFramebufferEXT")
  drv.glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  postDrawAction()
end
function glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  preDrawAction("glBlitNamedFramebuffer")
  drv.glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)
  postDrawAction()
end
function glCallList(list)
  preDrawAction("glCallList")
  drv.glCallList(list)
  postDrawAction()
end
function glCallLists(n, type, lists)
  preDrawAction("glCallLists")
  drv.glCallLists(n, type, lists)
  postDrawAction()
end
function glClear(mask)
  preDrawAction("glClear")
  drv.glClear(mask)
  postDrawAction()
end
function glGenerateMipmap(target)
  preDrawAction("glGenerateMipmap")
  drv.glGenerateMipmap(target)
  postDrawAction()
end
function glClearBufferfi(buffer, drawbuffer, depth, stencil)
  preDrawAction("glClearBufferfi")
  drv.glClearBufferfi(buffer, drawbuffer, depth, stencil)
  postDrawAction()
end
function glClearBufferfv(buffer, drawbuffer, value)
  preDrawAction("glClearBufferfv")
  drv.glClearBufferfv(buffer, drawbuffer, value)
  postDrawAction()
end
function glClearBufferiv(buffer, drawbuffer, value)
  preDrawAction("glClearBufferiv")
  drv.glClearBufferiv(buffer, drawbuffer, value)
  postDrawAction()
end
function glClearBufferuiv(buffer, drawbuffer, value)
  preDrawAction("glClearBufferuiv")
  drv.glClearBufferuiv(buffer, drawbuffer, value)
  postDrawAction()
end
function glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size)
  preDrawAction("glCopyBufferSubData")
  drv.glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size)
  postDrawAction()
end
function glCopyNamedBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size)
  preDrawAction("glCopyBufferSubData")
  drv.glCopyNamedBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size)
  postDrawAction()
end
function glCopyColorSubTable(target, start, x, y, width)
  preDrawAction("glCopyColorSubTable")
  drv.glCopyColorSubTable(target, start, x, y, width)
  postDrawAction()
end
function glCopyColorSubTableEXT(target, start, x, y, width)
  preDrawAction("glCopyColorSubTableEXT")
  drv.glCopyColorSubTableEXT(target, start, x, y, width)
  postDrawAction()
end
function glCopyColorTable(target, internalformat, x, y, width)
  preDrawAction("glCopyColorTable")
  drv.glCopyColorTable(target, internalformat, x, y, width)
  postDrawAction()
end
function glCopyColorTableSGI(target, internalformat, x, y, width)
  preDrawAction("glCopyColorTableSGI")
  drv.glCopyColorTableSGI(target, internalformat, x, y, width)
  postDrawAction()
end
function glCopyConvolutionFilter1D(target, internalformat, x, y, width)
  preDrawAction("glCopyConvolutionFilter1D")
  drv.glCopyConvolutionFilter1D(target, internalformat, x, y, width)
  postDrawAction()
end
function glCopyConvolutionFilter1DEXT(target, internalformat, x, y, width)
  preDrawAction("glCopyConvolutionFilter1DEXT")
  drv.glCopyConvolutionFilter1DEXT(target, internalformat, x, y, width)
  postDrawAction()
end
function glCopyConvolutionFilter2D(target, internalformat, x, y, width, height)
  preDrawAction("glCopyConvolutionFilter2D")
  drv.glCopyConvolutionFilter2D(target, internalformat, x, y, width, height)
  postDrawAction()
end
function glCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height)
  preDrawAction("glCopyConvolutionFilter2DEXT")
  drv.glCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height)
  postDrawAction()
end
function glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
  preDrawAction("glCopyImageSubData")
  drv.glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
  postDrawAction()
end
function glCopyImageSubDataNV(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
  preDrawAction("glCopyImageSubDataNV")
  drv.glCopyImageSubDataNV(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth)
  postDrawAction()
end
function glCopyMultiTexImage1DEXT(texunit, target, level, internalformat, x, y, width, border)
  preDrawAction("glCopyMultiTexImage1DEXT")
  drv.glCopyMultiTexImage1DEXT(texunit, target, level, internalformat, x, y, width, border)
  postDrawAction()
end
function glCopyMultiTexImage2DEXT(texunit, target, level, internalformat, x, y, width, height, border)
  preDrawAction("glCopyMultiTexImage2DEXT")
  drv.glCopyMultiTexImage2DEXT(texunit, target, level, internalformat, x, y, width, height, border)
  postDrawAction()
end
function glCopyMultiTexSubImage1DEXT(texunit, target, level, xoffset, x, y, width)
  preDrawAction("glCopyMultiTexSubImage1DEXT")
  drv.glCopyMultiTexSubImage1DEXT(texunit, target, level, xoffset, x, y, width)
  postDrawAction()
end
function glCopyMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, x, y, width, height)
  preDrawAction("glCopyMultiTexSubImage2DEXT")
  drv.glCopyMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, x, y, width, height)
  postDrawAction()
end
function glCopyMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyMultiTexSubImage3DEXT")
  drv.glCopyMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction()
end
function glCopyPathNV(resultPath, srcPath)
  preDrawAction("glCopyPathNV")
  drv.glCopyPathNV(resultPath, srcPath)
  postDrawAction()
end
function glCopyPixels(x, y, width, height, type)
  preDrawAction("glCopyPixels")
  drv.glCopyPixels(x, y, width, height, type)
  postDrawAction()
end
function glCopyTexImage1D(target, level, internalFormat, x, y, width, border)
  preDrawAction("glCopyTexImage1D")
  drv.glCopyTexImage1D(target, level, internalFormat, x, y, width, border)
  postDrawAction()
end
function glCopyTexImage1DEXT(target, level, internalFormat, x, y, width, border)
  preDrawAction("glCopyTexImage1DEXT")
  drv.glCopyTexImage1DEXT(target, level, internalFormat, x, y, width, border)
  postDrawAction()
end
function glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border)
  preDrawAction("glCopyTexImage2D")
  drv.glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border)
  postDrawAction()
end
function glCopyTexImage2DEXT(target, level, internalFormat, x, y, width, height, border)
  preDrawAction("glCopyTexImage2DEXT")
  drv.glCopyTexImage2DEXT(target, level, internalFormat, x, y, width, height, border)
  postDrawAction()
end
function glCopyTexSubImage1D(target, level, xoffset, x, y, width)
  preDrawAction("glCopyTexSubImage1D")
  drv.glCopyTexSubImage1D(target, level, xoffset, x, y, width)
  postDrawAction()
end
function glCopyTexSubImage1DEXT(target, level, xoffset, x, y, width)
  preDrawAction("glCopyTexSubImage1DEXT")
  drv.glCopyTexSubImage1DEXT(target, level, xoffset, x, y, width)
  postDrawAction()
end
function glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage2D")
  drv.glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height)
  postDrawAction()
end
function glCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage2DEXT")
  drv.glCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height)
  postDrawAction()
end
function glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage3D")
  drv.glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction()
end
function glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage3DEXT")
  drv.glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction()
end
function glCopyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyTexSubImage3DOES")
  drv.glCopyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction()
end
function glCopyTextureImage1DEXT(texture, target, level, internalformat, x, y, width, border)
  preDrawAction("glCopyTextureImage1DEXT")
  drv.glCopyTextureImage1DEXT(texture, target, level, internalformat, x, y, width, border)
  postDrawAction()
end
function glCopyTextureImage2DEXT(texture, target, level, internalformat, x, y, width, height, border)
  preDrawAction("glCopyTextureImage2DEXT")
  drv.glCopyTextureImage2DEXT(texture, target, level, internalformat, x, y, width, height, border)
  postDrawAction()
end
function glCopyTextureSubImage1DEXT(texture, target, level, xoffset, x, y, width)
  preDrawAction("glCopyTextureSubImage1DEXT")
  drv.glCopyTextureSubImage1DEXT(texture, target, level, xoffset, x, y, width)
  postDrawAction()
end
function glCopyTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, x, y, width, height)
  preDrawAction("glCopyTextureSubImage2DEXT")
  drv.glCopyTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, x, y, width, height)
  postDrawAction()
end
function glCopyTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, x, y, width, height)
  preDrawAction("glCopyTextureSubImage3DEXT")
  drv.glCopyTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, x, y, width, height)
  postDrawAction()
end
function glDispatchCompute(num_groups_x, num_groups_y, num_groups_z)
  preDrawAction("glDispatchCompute")
  drv.glDispatchCompute(num_groups_x, num_groups_y, num_groups_z)
  postDrawAction()
end
function glDispatchComputeIndirect(indirect)
  preDrawAction("glDispatchComputeIndirect")
  drv.glDispatchComputeIndirect(indirect)
  postDrawAction()
end
function glDrawArrays(mode, first, count)
  preDrawAction("glDrawArrays", mode, count)
  drv.glDrawArrays(mode, first, count)
  postDrawAction()
end
function glDrawArraysEXT(mode, first, count)
  preDrawAction("glDrawArraysEXT", mode, count)
  drv.glDrawArraysEXT(mode, first, count)
  postDrawAction()
end
function glDrawArraysIndirect(mode, indirect)
  preDrawAction("glDrawArraysIndirect", mode, 0)
  drv.glDrawArraysIndirect(mode, indirect)
  postDrawAction()
end
function glDrawArraysInstanced(mode, first, count, instancecount)
  preDrawAction("glDrawArraysInstanced", mode, count)
  drv.glDrawArraysInstanced(mode, first, count, instancecount)
  postDrawAction()
end
function glDrawArraysInstancedANGLE(mode, first, count, instancecount)
  preDrawAction("glDrawArraysInstancedANGLE", mode, count)
  drv.glDrawArraysInstancedANGLE(mode, first, count, instancecount)
  postDrawAction()
end
function glDrawArraysInstancedARB(mode, first, count, instancecount)
  preDrawAction("glDrawArraysInstancedARB", mode, count)
  drv.glDrawArraysInstancedARB(mode, first, count, instancecount)
  postDrawAction()
end
function glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance)
  preDrawAction("glDrawArraysInstancedBaseInstance", mode, count)
  drv.glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance)
  postDrawAction()
end
function glDrawArraysInstancedEXT(mode, first, count, instancecount)
  preDrawAction("glDrawArraysInstancedEXT", mode, count)
  drv.glDrawArraysInstancedEXT(mode, first, count, instancecount)
  postDrawAction()
end
function glDrawElements(mode, count, type, indices)
  preDrawAction("glDrawElements", mode, count)
  drv.glDrawElements(mode, count, type, indices)
  postDrawAction()
end
function glDrawElementsBaseVertex(mode, count, type, indices, basevertex)
  preDrawAction("glDrawElementsBaseVertex", mode, count)
  drv.glDrawElementsBaseVertex(mode, count, type, indices, basevertex)
  postDrawAction()
end
function glDrawElementsIndirect(mode, type, indirect)
  preDrawAction("glDrawElementsIndirect", mode, 0)
  drv.glDrawElementsIndirect(mode, type, indirect)
  postDrawAction()
end
function glDrawElementsInstanced(mode, count, type, indices, instancecount)
  preDrawAction("glDrawElementsInstanced", mode, count)
  drv.glDrawElementsInstanced(mode, count, type, indices, instancecount)
  postDrawAction()
end
function glDrawElementsInstancedANGLE(mode, count, type, indices, instancecount)
  preDrawAction("glDrawElementsInstancedANGLE", mode, count)
  drv.glDrawElementsInstancedANGLE(mode, count, type, indices, instancecount)
  postDrawAction()
end
function glDrawElementsInstancedARB(mode, count, type, indices, instancecount)
  preDrawAction("glDrawElementsInstancedARB", mode, count)
  drv.glDrawElementsInstancedARB(mode, count, type, indices, instancecount)
  postDrawAction()
end
function glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance)
  preDrawAction("glDrawElementsInstancedBaseInstance", mode, count)
  drv.glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance)
  postDrawAction()
end
function glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex)
  preDrawAction("glDrawElementsInstancedBaseVertex", mode, count)
  drv.glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex)
  postDrawAction()
end
function glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance)
  preDrawAction("glDrawElementsInstancedBaseVertexBaseInstance", mode, count)
  drv.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance)
  postDrawAction()
end
function glDrawElementsInstancedEXT(mode, count, type, indices, instancecount)
  preDrawAction("glDrawElementsInstancedEXT", mode, count)
  drv.glDrawElementsInstancedEXT(mode, count, type, indices, instancecount)
  postDrawAction()
end
function glDrawPixels(width, height, format, type, pixels)
  preDrawAction("glDrawPixels")
  drv.glDrawPixels(width, height, format, type, pixels)
  postDrawAction()
end
function glDrawRangeElements(mode, start, end_, count, type, indices)
  preDrawAction("glDrawRangeElements", mode, count)
  drv.glDrawRangeElements(mode, start, end_, count, type, indices)
  postDrawAction()
end
function glDrawRangeElementsBaseVertex(mode, start, end_, count, type, indices, basevertex)
  preDrawAction("glDrawRangeElementsBaseVertex", mode, count)
  drv.glDrawRangeElementsBaseVertex(mode, start, end_, count, type, indices, basevertex)
  postDrawAction()
end
function glDrawRangeElementsEXT(mode, start, end_, count, type, indices)
  preDrawAction("glDrawRangeElementsEXT", mode, count)
  drv.glDrawRangeElementsEXT(mode, start, end_, count, type, indices)
  postDrawAction()
end
function glDrawTexfOES(x, y, z, width, height)
  preDrawAction("glDrawTexfOES")
  drv.glDrawTexfOES(x, y, z, width, height)
  postDrawAction()
end
function glDrawTexfvOES(coords)
  preDrawAction("glDrawTexfvOES")
  drv.glDrawTexfvOES(coords)
  postDrawAction()
end
function glDrawTexiOES(x, y, z, width, height)
  preDrawAction("glDrawTexiOES")
  drv.glDrawTexiOES(x, y, z, width, height)
  postDrawAction()
end
function glDrawTexivOES(coords)
  preDrawAction("glDrawTexivOES")
  drv.glDrawTexivOES(coords)
  postDrawAction()
end
function glDrawTexsOES(x, y, z, width, height)
  preDrawAction("glDrawTexsOES")
  drv.glDrawTexsOES(x, y, z, width, height)
  postDrawAction()
end
function glDrawTexsvOES(coords)
  preDrawAction("glDrawTexsvOES")
  drv.glDrawTexsvOES(coords)
  postDrawAction()
end
function glDrawTexxOES(x, y, z, width, height)
  preDrawAction("glDrawTexxOES")
  drv.glDrawTexxOES(x, y, z, width, height)
  postDrawAction()
end
function glDrawTexxvOES(coords)
  preDrawAction("glDrawTexxvOES")
  drv.glDrawTexxvOES(coords)
  postDrawAction()
end
function glDrawTransformFeedback(mode, id)
  preDrawAction("glDrawTransformFeedback", mode, 0)
  drv.glDrawTransformFeedback(mode, id)
  postDrawAction()
end
function glDrawTransformFeedbackInstanced(mode, id, instancecount)
  preDrawAction("glDrawTransformFeedbackInstanced", mode, 0)
  drv.glDrawTransformFeedbackInstanced(mode, id, instancecount)
  postDrawAction()
end
function glDrawTransformFeedbackNV(mode, id)
  preDrawAction("glDrawTransformFeedbackNV")
  drv.glDrawTransformFeedbackNV(mode, id)
  postDrawAction()
end
function glDrawTransformFeedbackStream(mode, id, stream)
  preDrawAction("glDrawTransformFeedbackStream")
  drv.glDrawTransformFeedbackStream(mode, id, stream)
  postDrawAction()
end
function glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount)
  preDrawAction("glDrawTransformFeedbackStreamInstanced")
  drv.glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount)
  postDrawAction()
end
function glMultiDrawArrays(mode, first, count, drawcount)
  preDrawAction("glMultiDrawArrays", mode, 0)
  drv.glMultiDrawArrays(mode, first, count, drawcount)
  postDrawAction()
end
function glMultiDrawArraysEXT(mode, first, count, drawcount)
  preDrawAction("glMultiDrawArraysEXT", mode, 0)
  drv.glMultiDrawArraysEXT(mode, first, count, drawcount)
  postDrawAction()
end
function glMultiDrawArraysIndirect(mode, indirect, drawcount, stride)
  preDrawAction("glMultiDrawArraysIndirect", mode, 0)
  drv.glMultiDrawArraysIndirect(mode, indirect, drawcount, stride)
  postDrawAction()
end
function glMultiDrawArraysIndirectAMD(mode, indirect, drawcount, stride)
  preDrawAction("glMultiDrawArraysIndirectAMD", mode, 0)
  drv.glMultiDrawArraysIndirectAMD(mode, indirect, drawcount, stride)
  postDrawAction()
end
function glMultiDrawElements(mode, count, type, indices, drawcount)
  preDrawAction("glMultiDrawElements", mode, 0)
  drv.glMultiDrawElements(mode, count, type, indices, drawcount)
  postDrawAction()
end
function glMultiDrawElementsEXT(mode, count, type, indices, drawcount)
  preDrawAction("glMultiDrawElementsEXT", mode, 0)
  drv.glMultiDrawElementsEXT(mode, count, type, indices, drawcount)
  postDrawAction()
end
function glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride)
  preDrawAction("glMultiDrawElementsIndirect", mode, 0)
  drv.glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride)
  postDrawAction()
end
function glMultiDrawElementsIndirectAMD(mode, type, indirect, drawcount, stride)
  preDrawAction("glMultiDrawElementsIndirectAMD", mode, 0)
  drv.glMultiDrawElementsIndirectAMD(mode, type, indirect, drawcount, stride)
  postDrawAction()
end
function glNamedCopyBufferSubDataEXT(readBuffer, writeBuffer, readOffset, writeOffset, size)
  preDrawAction("glNamedCopyBufferSubDataEXT")
  drv.glNamedCopyBufferSubDataEXT(readBuffer, writeBuffer, readOffset, writeOffset, size)
  postDrawAction()
end

function glUseProgram(prog)
  preProgramAction(prog)
  drv.glUseProgram(prog)
  postProgramAction(prog)
end
