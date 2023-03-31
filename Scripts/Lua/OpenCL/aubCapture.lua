-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================


-- ***************************************************************
-- ******************** !!!! SETTINGS !!!! ***********************
-- ***************************************************************
-- ********SET YOUR OWN SCRIPT TO START/STOP AUB CAPTURE *********
startCmd = "aubStart.bat"
stopCmd = "aubStop.bat"
-- *****************SET START AND STOP FRAME *********************
startKernel = 1
stopKernel = 1
-- ***************************************************************
-- ***************************************************************
-- ***************************************************************


-- **************************** Globals **************************
kernelNr = 0

-- ************************** AUB Capture ************************
-- Start/Stop aub capture
function startAubCapture()
  os.execute(startCmd)
end

function stopAubCapture()
  os.execute(stopCmd)
end

-- **************************** Actions **************************

-- End frame actions
function preKernelAction()
end

function postKernelAction()
  kernelNr = kernelNr + 1
  if startKernel == kernelNr then
    startAubCapture()
  end
  if stopKernel + 1 == kernelNr then
    stopAubCapture()
  end
end

-- ***************** Functions triggered by GITS *********************
function clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset,
                                global_work_size, local_work_size, num_events_in_wait_list,
                                event_wait_list, event)
  preKernelAction()
  drvCl.clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset,
                             global_work_size, local_work_size, num_events_in_wait_list,
                             event_wait_list, event)
  postKernelAction()
end

-- Events
function gitsProgramStart()
end

function gitsProgramExit()
end

function gitsStateRestoreBegin()
end

function gitsStateRestoreEnd()
end
