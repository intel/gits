-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

function oclocInvoke(argc, argv, numSources, sources, sourceLens, sourcesNames,
                     numInputHeaders, dataInputHeaders, lenInputHeaders,
                     nameInputHeaders, numOutputs, dataOutputs, lenOutputs,
                     nameOutputs)
    for k,v in pairs(argv) do
        if v == "-options" then
            argv[k + 1] = argv[k + 1] .. gits.getArgsStr()
        end
    end
    return drvOcloc.oclocInvoke(argc, argv, numSources, sources, sourceLens, sourcesNames,
                     numInputHeaders, dataInputHeaders, lenInputHeaders,
                     nameInputHeaders, numOutputs, dataOutputs, lenOutputs,
                     nameOutputs)
end
