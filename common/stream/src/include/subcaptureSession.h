// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandSerializer.h"
#include "restoredContents.h"
#include "streamHeader.h"
#include "streamWriter.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gits {
namespace stream {

// Subcapture state shared by the subcapture recorders of all APIs in a player. The subcapture
// of a stream recorded with several APIs is one stream, and its frames are those of the API that
// ends frames first: in an application rendering with one API and computing with another, only
// one of them presents.
class SubcaptureSession {
public:
  // An API taking part in a subcapture of a stream with several APIs.
  class Participant {
  public:
    virtual ~Participant() = default;
    // Called for every participant before any writes its state: the API completes the work whose
    // results other APIs read back, and throws for a state the subcapture cannot restore.
    virtual void PrepareRestoreState() {}
    // Writes the state of the API at the end of the frame before the range.
    virtual void RestoreState() = 0;
    // Advances the range of the API to the next frame.
    virtual void EndFrame() = 0;
  };

  // The session is never destroyed: the recorders close the stream when the player exits, in an
  // order that is not known.
  static SubcaptureSession& Get();
  SubcaptureSession(const SubcaptureSession&) = delete;
  SubcaptureSession& operator=(const SubcaptureSession&) = delete;

  // Whether the stream being replayed was recorded with several APIs, whose subcaptures the
  // session drives together.
  static bool IsShared();

  // The analysis file of `api` for a subcapture of `frames` of the stream in `streamDir`. A
  // stream with several APIs has an analysis file per API.
  static std::string AnalysisFileName(const std::filesystem::path& streamDir,
                                      const std::string& frames,
                                      StreamHeader::Api api);
  // The analysis files of all APIs of the stream.
  static std::vector<std::string> AnalysisFileNames(const std::filesystem::path& streamDir,
                                                    const std::string& frames);

  // Opens the subcapture stream on the first call; later calls share it.
  void Open(const std::filesystem::path& path, CompressionType compressionType);
  bool IsOpen() const {
    return m_Writer != nullptr;
  }
  const std::filesystem::path& GetPath() const {
    return m_Path;
  }
  // Ignored once the stream is closed.
  void Record(const CommandSerializer& serializer);
  // Returns whether this call closed the stream.
  bool Close();

  uint64_t CreateStateRestoreKey() {
    return ++m_NextStateRestoreKey;
  }
  uint64_t CreateStateRestoreObjectKey() {
    return ++m_NextStateRestoreObjectKey;
  }

  // Participants restore their state in increasing `restoreOrder`. A range starting at
  // `startFrame` restores the state at the end of the frame before it. Returns the frame being
  // replayed, which is past the first one for an API whose first command follows a frame end.
  uint32_t AddParticipant(Participant* participant, unsigned restoreOrder, uint32_t startFrame);
  void RemoveParticipant(Participant* participant);
  // Ends the frame for every participant, after restoring their state at the end of the frame
  // before the range. Throws when an API other than the one that ended the first frame ends one.
  void EndFrame(StreamHeader::Api api);

  // Exports that the other API may import, identified by the key of the exporting command, of the
  // object `ownerKey`. The exporting API restores an export only while that object exists.
  void AddExport(uint64_t exportKey, uint64_t ownerKey);
  void RemoveExports(uint64_t ownerKey);
  bool IsExportRestored(uint64_t exportKey) const;
  // The exporting API reports in PrepareRestoreState the exported objects whose contents it cannot
  // restore. It restores first, so it is prepared before the importing API, which checks them.
  void SetContentsNotRestored(uint64_t ownerKey, std::string reason);
  // Why the contents of the object of export `exportKey` are not restored; nullptr when they are.
  const std::string* ContentsNotRestored(uint64_t exportKey) const;
  // The exporting API also reports, for each exported object it restores, which of its contents it
  // restores, as only the resources bound to it whose contents it reads back restore them.
  void SetRestoredContents(uint64_t ownerKey, RestoredContents contents);
  // nullptr when the exporting API reported none for the object of export `exportKey`.
  const RestoredContents* GetRestoredContents(uint64_t exportKey) const;
  // The importing API reports in PrepareRestoreState the range of an object of export `exportKey`
  // that it relies on the exporting API to restore. The exporting API then fails its state restore
  // when it cannot restore a resource bound over that range.
  void RequireContents(uint64_t exportKey, ContentRange range);
  bool IsContentRequired(uint64_t ownerKey, ContentRange range) const;
  // The layout in which the importing API hands an image back to the exporting API, identified by
  // the export of its object and its offset in that object. It is the OpenGL layout of
  // GL_EXT_semaphore that a semaphore signal names. The exporting API clears it when it releases
  // the image again, and reads it for an image released at the restore, whose layout it gives.
  void SetHandBackLayout(uint64_t exportKey, uint64_t offset, uint32_t layout);
  void ClearHandBackLayout(uint64_t ownerKey, uint64_t offset);
  std::optional<uint32_t> GetHandBackLayout(uint64_t ownerKey, uint64_t offset) const;

  // Binary semaphores whose payload one API exports and the other imports, identified by the key
  // of the exporting command. Both APIs signal and wait on the shared payload, so only their
  // operations together give its state. It starts as the state of the semaphore at its first
  // export; then the exporting API reports the state of its semaphore, the importing API that of
  // its import.
  void AddSemaphoreExport(uint64_t exportKey, uint64_t semaphoreKey, bool signaled);
  void ImportSemaphore(uint64_t exportKey);
  void SetSemaphoreSignaled(uint64_t semaphoreKey, bool signaled);
  void SetImportedSemaphoreSignaled(uint64_t exportKey, bool signaled);
  // Returns nullopt for a semaphore that no other API imported.
  std::optional<bool> GetImportedSemaphoreSignaled(uint64_t semaphoreKey) const;

  static constexpr uint64_t STATE_RESTORE_KEY_MASK = uint64_t{1} << 63;

private:
  SubcaptureSession() = default;

  struct Entry {
    Participant* Value{};
    unsigned RestoreOrder{};
  };

  struct SharedSemaphore {
    bool Signaled{};
    bool Imported{};
  };
  // The owner of each export, by export key.
  std::unordered_map<uint64_t, uint64_t> m_Exports;
  // The reason the contents are not restored, by owner key.
  std::unordered_map<uint64_t, std::string> m_ContentsNotRestored;
  // By owner key.
  std::unordered_map<uint64_t, RestoredContents> m_RestoredContents;
  // By owner key and offset.
  std::map<std::pair<uint64_t, uint64_t>, uint32_t> m_HandBackLayouts;
  std::unordered_map<uint64_t, uint64_t> m_SemaphoreExports;
  std::unordered_map<uint64_t, SharedSemaphore> m_SharedSemaphores;

  std::unique_ptr<StreamWriter> m_Writer;
  std::filesystem::path m_Path;
  bool m_Closed{};
  uint64_t m_NextStateRestoreKey{STATE_RESTORE_KEY_MASK};
  uint64_t m_NextStateRestoreObjectKey{STATE_RESTORE_KEY_MASK};
  std::vector<Entry> m_Participants;
  uint32_t m_StartFrame{1};
  uint32_t m_CurrentFrame{1};
  StreamHeader::Api m_FrameApi{StreamHeader::Api::API_NOT_SET};
};

} // namespace stream
} // namespace gits
