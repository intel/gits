// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

namespace gits {
namespace stream {

// Matches Windows handles that one API exports and another API imports. A handle value is valid
// only in the process that created it, so the stream stores the key of the command that exported
// the handle, and the player imports the handle its replay of that command returned.
//
// Each export belongs to the object it was exported from, identified by its key. A KMT handle
// export is released when that object is destroyed. An NT handle stays importable until every
// handle to it is closed, which GITS does not track, so recorded exports are kept until only the
// one duplicate GITS keeps of their object is left, and a replayed one until CloseReplayedExports.
class ExternalHandleService {
public:
  // Never destroyed, as recorders of other APIs may still record while static objects are
  // destroyed.
  static ExternalHandleService& Get();
  ExternalHandleService(const ExternalHandleService&) = delete;
  ExternalHandleService& operator=(const ExternalHandleService&) = delete;

  // An NT handle is matched by the object it refers to, as the application may import a duplicate
  // of it. Other handles are global names and are matched by value.
  void AddRecordedExport(uint64_t ownerKey, uint64_t exportKey, void* handle, bool ntHandle);
  void RemoveRecordedExports(uint64_t ownerKey);
  // The key of the latest export of the handle that precedes the command importing it, which is
  // the one replayed before the import, as another thread may export the object again after the
  // import reserved its key. Returns 0 for a handle that no earlier recorded command exported.
  uint64_t GetRecordedExportKey(void* handle, uint64_t importKey) const;

  void AddReplayedExport(uint64_t ownerKey, uint64_t exportKey, void* handle, bool ntHandle);
  void RemoveReplayedExports(uint64_t ownerKey);
  // Closes the replayed NT handles once nothing in the stream can import them anymore.
  void CloseReplayedExports();
  // Returns nullptr for a key whose command was not replayed.
  void* GetReplayedExport(uint64_t exportKey) const;

private:
  ExternalHandleService() = default;
  void PruneRecordedExports();

  struct Export {
    uint64_t OwnerKey{};
    uint64_t ExportKey{};
    void* Handle{};
    bool NtHandle{};
    bool OwnerDestroyed{};
  };

  mutable std::mutex m_Mutex;
  std::vector<Export> m_RecordedExports;
  std::vector<Export> m_ReplayedExports;
};

} // namespace stream
} // namespace gits
