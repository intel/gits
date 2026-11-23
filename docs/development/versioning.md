---
icon: material/tag-outline
title: Versioning
---

# Versioning

Since 3.0, a GITS version is `MAJOR.MINOR.PATCH`. MAJOR and MINOR follow stream compatibility (see [When to bump the version](#when-to-bump-the-version)), and PATCH counts release builds. Development builds and build metadata use [SemVer](https://semver.org/) syntax, so tools that parse SemVer sort GITS versions correctly:

| Build             | Version                          | Package label                          |
| ----------------- | -------------------------------- | -------------------------------------- |
| Release           | `3.0.123`                        | `GITS_3.0.123`                         |
| Development       | `3.0.999-dev+pr-456.sha-abc1234` | `GITS_development_pr-456.sha-abc1234`  |
| Local development | `3.0.999-dev+local.sha-abc1234`  | -                                      |

- **MAJOR** and **MINOR** are set in `common/legacy/include/gitsVersion.h`.
- **PATCH** is not edited by hand. Release builds get it from the CI build number (0 to 998); development builds always use 999.
- `-dev` marks a development build. In SemVer order it sits above every release of its line and below the next MINOR: `3.0.998 < 3.0.999-dev < 3.1.0`.
- The build metadata after `+` identifies the source: the pull request (`pr-<N>`) or CI build (`qb-<N>`) plus the commit, or `local.sha-<commit>` for a local build, with `-dirty` when it has uncommitted changes. Configure CMake with `-DGITS_BUILD_ID=<metadata>` to override it.

`gitsPlayer --version`, the recorder log, the HUD and the `diag.gits.version` stream property show the full version. Development builds also store the full commit in `diag.gits.commit`.

## When to bump the version

- Bump **MINOR** when a change needs a new stream feature gate, i.e. the player must behave differently for streams recorded before the change.
- Bump **MAJOR** for changes that older players cannot handle at all.
- Every bump restarts PATCH at 0, so the CI build offset must be reset at the same time (see below).

## Version in streams

Streams start with a fixed-width version label `GITS_dd.dd.dd.ddd`, which every GITS version, including 2.x players, can read. Versions from 3.0 are stored with the third field set to zero:

| Version       | Stream label        |
| ------------- | ------------------- |
| `3.0.123`     | `GITS_03.00.00.123` |
| `3.0.999-dev` | `GITS_03.00.00.999` |
| 2.x (legacy)  | `GITS_02.00.13.123` |

The label is compared as a packed 64-bit number. This order matches SemVer order, and every 3.x version sorts above every 2.x version. GITS displays versions below 3.0 in their original legacy form, `GITS_02.00.13.123`.

Stream feature gates compare against the recording version with `stream_older_than()`. For versions from 3.0, use `GITS_MAKE_SEMVER(major, minor, patch)`. It produces the matching stream label:

```cpp
#define GITS_NEW_FEATURE GITS_MAKE_SEMVER(3, 1, 0)
```

Existing gates for 2.x versions (`GITS_MAKE_VERSION3`, `GITS_MAKE_VERSION4`) stay unchanged.


