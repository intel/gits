// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// GLuint <-> GITSKey, bucketed per GL object-name namespace - see
// docs/development/opengl2-object-name-mapping-plan.md. Structurally the array-of-namespaces
// counterpart of ContextHandleMapService (read that class's own header comment first; this one
// only documents what's different): same recorder-direction (name -> key)/player-direction
// (key -> name) split, same mutex/map/warned-set shape, same "Register() takes an
// already-minted key" contract - just repeated once per GLObjectKind bucket below instead of
// once total.
//
// The bucketing itself is the one genuinely new problem here, absent from ContextHandleMapService:
// every GL object-name namespace (texture, buffer, program, shader, framebuffer, vertex-array)
// uses the exact same C++ type, GLuint - a texture name 1 and a buffer name 1 are different
// objects and must not collide in one flat map. Vulkan's HandleMapService buckets its
// recorder-direction map per C++ type (HandleTypeIndex<T>()), which works there because
// VkBuffer/VkImage/VkPipeline/... are all distinct C++ types; that trick cannot distinguish two
// same-typed GLuint namespaces, so this service takes an explicit GLObjectKind bucket-selector
// parameter on every call instead. See the plan doc's namespace-tag decision for the alternatives
// considered (e.g. giving GLObjectKind its own dedicated C++ type per namespace, à la Vulkan) and
// why a runtime enum parameter was chosen over any of them.

#include "arguments.h"
#include "glTypes.h"

#include <array>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace gits::opengl2 {

// Which GL object-name namespace a GLuint value belongs to - see this header's own comment
// above. Mirrors object_name_lifecycle.py's GLObjectKind StrEnum value-for-value; codegen
// (recorderWrappersAuto.cpp.mako/commandRunnersAuto.cpp.mako) prints `GLObjectKind::<value>`
// directly from that Python enum's `.value`, so the two must stay in lockstep - a mismatch
// would just fail to compile the generated call site, not silently misbehave.
//
// Framebuffer/VertexArray now have real ObjectNameMapService callers too (see the FBO-pipeline
// round, docs/development/opengl2-player-skeleton-plan.md's Decision 10: glGenFramebuffers/
// glGenVertexArrays create names, glBindFramebuffer/glBindVertexArray look them up) - both kinds
// were declared here ahead of that work specifically so this service's bucket array did not
// need to grow again the moment either Gen command was enabled, which is exactly what happened.
//
// Query was added for glGenQueries/glDeleteQueries/glBeginQuery (see
// modern-opengl-backend-architecture.md's "Reduce replay warning noise" decision).
//
// Renderbuffer/Sampler/ProgramPipeline were added for glGenRenderbuffers/glGenSamplers/
// glGenProgramPipelines and their Bind/Delete counterparts: glBindRenderbuffer/glBindSampler/
// glBindProgramPipeline were already enabled (scalar-only) with no lifecycle tracking for
// their own object namespace at all, so their GLuint name was passed straight through to the
// replay driver unresolved - see object_name_lifecycle.py's matching GLObjectKind comment.
//
// TransformFeedback was added for glGenTransformFeedbacks/glDeleteTransformFeedbacks/
// glBindTransformFeedback/glIsTransformFeedback/glDrawTransformFeedback(Stream(Instanced)) -
// see object_name_lifecycle.py's matching GLObjectKind comment.
//
// Share-group identity (see modern-opengl-backend-architecture.md's share-group decision
// record and common/services/shareGroupService.h): a GLuint object name is unique only within
// a share group, not process-globally - two independent, non-sharing contexts can each have a
// buffer named 1. Only the recorder-direction (name -> key) methods below take a `shareGroup`
// parameter; the player-direction (key -> name) methods do not need one, because a GITSKey is
// already created uniquely per registered object (see CaptureManager::CreateHandleKey()) and so
// never collides across share groups on its own - only the *name*-keyed forward lookup does.
//
// `shareGroup` is an opaque bucketing identity from this service's point of view - it does not
// have to be a true, multi-context share-group id. Framebuffer/VertexArray/ProgramPipeline/
// Query/TransformFeedback objects are never shared between contexts even within a share group
// that shares everything else (see the GL spec's "Shared Objects and Multiple Contexts" rule),
// so callers pass those five kinds a raw per-context identity instead of a real share-group id
// - see recorder/include/handleArgumentUpdaters.h's GroupIdentityFor(), the only place that
// decides which identity a given GLObjectKind gets. This service itself stays kind-agnostic
// about that choice; it only ever compares whatever uint64_t it is given for equality.
enum class GLObjectKind : uint8_t {
  Texture,
  Buffer,
  Program,
  Shader,
  Framebuffer,
  VertexArray,
  Query,
  Renderbuffer,
  Sampler,
  ProgramPipeline,
  TransformFeedback,
  kCount, // Not a real bucket - array size sentinel only.
};

class ObjectNameMapService {
public:
  static ObjectNameMapService& Get();

  // Registers a newly-created GITSKey for a newly-created GL object name in the given
  // namespace and share group (see CurrentShareGroup() in
  // recorder/include/handleArgumentUpdaters.h for how callers determine `shareGroup`). Called
  // only for the object-name-creating commands (glGenTextures/glGenBuffers' array elements,
  // glCreateProgram/glCreateShader's return value) - see
  // recorder/include/handleArgumentUpdaters.h's UpdateOutputHandle/UpdateOutputHandleArray. A
  // zero name (a failed create) is not registered.
  void Register(GLObjectKind kind, uint64_t shareGroup, GLuint name, GITSKey key);

  // Looks up the key for an existing name in the given namespace and share group
  // (glBindTexture/glBindBuffer/glUseProgram/glBindAttribLocation/glGetUniformLocation's
  // object-name parameter, and glDeleteTextures/glDeleteProgram/glDeleteShader's
  // about-to-be-destroyed name). Returns 0 (a never-valid key) and logs a single warning per
  // unique never-seen (kind, shareGroup, name) tuple if it is not registered - see
  // handleArgumentUpdaters.h's UpdateHandle/UpdateHandleArray. A zero name returns 0 silently
  // (e.g. glBindTexture(target, 0) unbinds the target - a normal, expected call, not a missing
  // registration).
  GITSKey GetKey(GLObjectKind kind, uint64_t shareGroup, GLuint name);

  // Removes a destroyed name's mapping (glDeleteTextures/glDeleteProgram/glDeleteShader) in the
  // given namespace and share group, so a later create that happens to reuse the same GLuint
  // value (in the same or a different share group) creates a fresh key instead of silently
  // reusing the destroyed object's identity. No-op for an unmapped/zero name. Recorder-only;
  // the player has no corresponding call - mirrors ContextHandleMapService::Erase's own
  // "recorder-only" precedent exactly (see that method's comment): the player-side map is
  // discarded wholesale at process exit either way, so there is nothing a player-side delete
  // would protect against within a single replay. Named Delete (not Erase, unlike
  // ContextHandleMapService's own method - see object_name_lifecycle.py's ObjectNameRole
  // docstring for why) to match the GL API's own vocabulary for this operation
  // (glDeleteTextures/glDeleteProgram/glDeleteShader) rather than reading as an unrelated
  // container-erase detail.
  //
  // GLObjectKind::Program/GLObjectKind::Shader deliberately keep their forward (name -> key)
  // mapping alive here instead of erasing it immediately - see the GL spec's deferred-deletion
  // rule for both object types ("a shader/program object will not be deleted ... until it is no
  // longer attached to any program object" / "... no longer in use"): unlike a texture/buffer/
  // framebuffer/vertex-array/query/renderbuffer/sampler name (whose spec language invalidates
  // the name for every purpose the instant glDelete* runs), a deleted-but-still-attached
  // shader or a deleted-but-still-current program remains a perfectly legal target for further
  // captured commands (glDetachShader's `shader`, a DSA glProgramUniform*'s `program`, ...)
  // until GL actually reclaims the name. Erasing eagerly here (this class's original
  // behavior, and every other kind's behavior still) made exactly that legal window resolve to
  // key 0 - the same class of bug UniformLocationMapService::PurgeProgram's own deferred-purge
  // fix addressed for uniform locations, just one layer up at the name-to-key mapping itself.
  //
  // Correctness of simply never eagerly erasing (rather than replicating
  // UniformLocationMapService's full is-it-still-referenced/PendingDeletion tracking here
  // too): a driver only ever hands out a given (shareGroup, name) pair again once it has
  // actually reclaimed it, which per the same deferred-deletion rule cannot happen while the
  // old object is still attached/current - so there is no window in which a REUSED name could
  // collide with the still-registered old mapping. Register() above already overwrites
  // NameToKey[nameKey] unconditionally on that eventual reuse, so the fresh key always wins the
  // moment it is actually needed; until then, this mapping simply (and correctly) continues to
  // resolve any legal reference to the old, not-yet-reclaimed object. The one accepted
  // trade-off: a Program/Shader name that is deleted and never reused for the rest of this
  // capture leaks its map entry for the capture's remaining lifetime - deliberately accepted,
  // not a correctness issue, and no worse than several other already-documented "player-side
  // cleanup is not load-bearing within a single run" trade-offs elsewhere in this backend
  // (e.g. ContextHandleMapService::Erase's own comment).
  //
  // GLObjectKind::TransformFeedback keeps its mapping alive here too, for a related but
  // DIFFERENT reason than Program/Shader above:
  // glDeleteTransformFeedbacks itself can fail outright with GL_INVALID_OPERATION if any name
  // in `ids` names the currently bound AND active (i.e. mid glBeginTransformFeedback/
  // glEndTransformFeedback) transform feedback object - unlike Program/Shader's genuine
  // deferred-then-eventually-reclaimed deletion, a REJECTED glDeleteTransformFeedbacks call
  // does not mark the object for deletion at all: the name remains fully, immediately valid,
  // exactly as if the call had never been made. glDeleteTransformFeedbacks is void-returning,
  // so recorderWrappersAuto.cpp.mako's wrapper has no return value to gate this call on the
  // way it already does for a non-void delete-role command (see that template's own
  // `erase_param`/`result` handling) - and checking glGetError() immediately after would risk
  // consuming/clearing an unrelated, already-pending error the application itself has not yet
  // observed, the same GL-error-state-fidelity hazard already avoided elsewhere in this
  // backend (see e.g. clientMemoryCapture.cpp's query-buffer-binding guard, which takes the
  // same precaution for the same reason). So, exactly like Program/Shader above, this simply
  // never erases a TransformFeedback mapping at all: a still-legal reference (the
  // overwhelmingly common case - most calls succeed) keeps resolving correctly regardless of
  // whether this specific call happened to fail, and an eventual reused-name create still
  // overwrites it via Register() exactly as before. The same one-line trade-off as
  // Program/Shader applies: a deleted-and-never-reused TransformFeedback name leaks its map
  // entry for the capture's remaining lifetime.
  void Delete(GLObjectKind kind, uint64_t shareGroup, GLuint name);

  // ---- Player direction: (kind, GITSKey) -> live GLuint, established as the stream replays. --

  // Registers the live GLuint name assigned by a real glGenTextures/glGenBuffers/
  // glCreateProgram/glCreateShader call under the key that was already decoded onto the same
  // field - see player/include/handleArgumentUpdaters.h's RegisterCreatedHandle/
  // RegisterCreatedHandleArray. A zero key or zero name is a no-op.
  void SetHandle(GLObjectKind kind, GITSKey key, GLuint name);

  // Looks up the live GLuint name for an existing key in the given namespace (every other
  // GL-object-name use) - see player/include/handleArgumentUpdaters.h's ResolveHandle/
  // ResolveHandleArray. Returns 0 and logs a single warning per unique never-seen (kind, key)
  // pair if it is not registered. A zero key returns 0 silently (mirrors GetKey's zero-name
  // handling above).
  GLuint GetHandle(GLObjectKind kind, GITSKey key);

private:
  ObjectNameMapService() = default;

  // Recorder-direction (name -> key) map key: a GLuint name is only unique within its share
  // group (see this header's own comment above), so the forward lookup must be keyed on both.
  struct NameKey {
    uint64_t ShareGroup;
    GLuint Name;
    bool operator==(const NameKey& other) const {
      return ShareGroup == other.ShareGroup && Name == other.Name;
    }
  };
  struct NameKeyHash {
    size_t operator()(const NameKey& key) const {
      return std::hash<uint64_t>()(key.ShareGroup) ^ (std::hash<GLuint>()(key.Name) << 1);
    }
  };

  // One independent {mutex, both maps, both warned-sets} bucket per GLObjectKind - see this
  // header's own comment for why a namespace tag is unavoidable here (unlike
  // ContextHandleMapService, which has exactly one namespace and needs no bucketing at all).
  // KeyToName/WarnedMissingKeys are plain GITSKey-keyed maps, not share-group-keyed - see this
  // header's own comment above for why only the name-keyed forward direction needs it.
  struct Bucket {
    std::mutex Mutex;
    std::unordered_map<NameKey, GITSKey, NameKeyHash> NameToKey;
    std::unordered_set<NameKey, NameKeyHash> WarnedMissingNames;
    std::unordered_map<GITSKey, GLuint> KeyToName;
    std::unordered_set<GITSKey> WarnedMissingKeys;
  };

  Bucket& BucketFor(GLObjectKind kind);

  std::array<Bucket, static_cast<size_t>(GLObjectKind::kCount)> m_Buckets;
};

} // namespace gits::opengl2
