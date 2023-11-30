# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import argparse
import logging
import subprocess
import sys
import os
import re
from enum import Enum
from pathlib import Path

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)


def get_script_path():
    return os.path.dirname(os.path.realpath(sys.argv[0]))


def get_root_directory() -> Path:
    return Path(get_script_path()).parent


GIT_VERSION_PARALLEL = "2.9.0"


class GitCheck:
    def __init__(self) -> None:
        self.current_version = str(self.get_git_version())

    def get_git_version(self):
        return subprocess.run(
            "git --version", stdout=subprocess.PIPE, check=True, shell=True
        ).stdout

    def get_versions(self, version):
        m = re.search(r"(\d+)\.(\d+)\.(\d+)", version)
        if m:
            return int(m.group(1)), int(m.group(2)), int(m.group(3))

    def check(self) -> bool:
        min_major, min_minor, min_patch = self.get_versions(GIT_VERSION_PARALLEL)
        cur_major, cur_minor, cur_patch = self.get_versions(self.current_version)
        if cur_major > min_major:
            return True
        if cur_major == min_major and cur_minor > min_minor:
            return True
        if cur_major == min_major and cur_minor == min_minor and cur_patch >= min_patch:
            return True
        logger.info(f"Current GIT version {cur_major}.{cur_minor}.{cur_patch}")
        return False


THIRD_PARTY_PATH = get_root_directory() / "third_party"
PATCH_PATH = THIRD_PARTY_PATH / "patch"
USE_PARALLEL_GIT = GitCheck().check()


class OperatingSystem(Enum):
    WINDOWS = 1
    LINUX = 2


class Repository:
    def __init__(self) -> None:
        self._repository_path = None
        self.name = ""
        self.url = ""
        self.branch = ""
        self.commit_id = ""
        self.os = None
        self.long_paths = False
        self.modules = []
        if not THIRD_PARTY_PATH.exists():
            THIRD_PARTY_PATH.mkdir()
        self.set_branch()
        self.set_commit()
        self.init()

    @property
    def repository_path(self) -> Path:
        if not self._repository_path:
            self._repository_path = THIRD_PARTY_PATH / self.name
            logger.debug("Component path: %s", str(self._repository_path))
        return self._repository_path

    def init(self) -> None:
        pass

    def set_branch(self):
        self.branch = "master"

    def build(self):
        pass

    def set_commit(self):
        pass

    def cleanup(self):
        pass

    def execute(self, cmd, cwd=None, must_pass=True):
        logger.info("Running command line: %s", cmd)
        process = subprocess.run(
            cmd,
            cwd=cwd if cwd else self.repository_path,
            shell=True,
            stderr=subprocess.PIPE,
        )
        if must_pass and process.returncode != 0:
            logger.error(process.stdout)
            logger.error(process.stderr)
        if must_pass:
            try:
                process.check_returncode()
            except:
                exit(1)
        return process

    def apply_patches(self):
        for i in self.modules:
            for patch in Path(PATCH_PATH / i).glob("*.patch"):
                logger.info(f"applying patch {patch}")
                process = self.execute(
                    f"git apply --check {str(patch)}",
                    cwd=self.repository_path.parent / i,
                    must_pass=False,
                )
                if len(process.stderr) == 0:
                    self.execute(
                        f"git apply {patch}", cwd=self.repository_path.parent / i
                    )
                else:
                    logger.info(f"Patch {patch} is already applied")

    def clone(self):
        clone = True
        if self.repository_path.exists():
            logger.info(f"Repository is already cloned: {self.name}")
            clone = False
        else:
            self.repository_path.mkdir()
        if clone:
            clone_cmd = [
                f"git clone {self.url} --branch {self.branch} --recursive {self.name}"
            ]
            if USE_PARALLEL_GIT:
                clone_cmd.append("-j 8")
            if self.long_paths:
                clone_cmd.append("-c core.longpaths=true")
            self.execute(
                " ".join(clone_cmd),
                cwd=self.repository_path.parent,
            )
            if self.commit_id:
                self.execute(f"git reset --hard {self.commit_id}")
        self.scan_repositories()

    def scan_repositories(self) -> None:
        modules_paths = self.repository_path.rglob(".git")
        for submodule_path in modules_paths:
            self.modules.append(
                submodule_path.relative_to(self.repository_path.parent).parent
            )
        logger.debug(str(self.modules))

    def check_skip_os(self) -> bool:
        return (self.os == OperatingSystem.WINDOWS and os.name != "nt") or (
            self.os == OperatingSystem.LINUX and os.name == "nt"
        )

    def install(self):
        try:
            if self.check_skip_os():
                logger.debug("Skipping %s installation due to os limitation", self.name)
                return 0
            install = True
            if self.repository_path.exists():
                logger.info(f"Repository is already cloned: {self.name}")
                install = False
            if install:
                self.clone()
            self.apply_patches()
            if install:
                self.build()
                self.cleanup()
        except subprocess.CalledProcessError:
            logger.exception("Issue installing repository.")
            return -1
        return 0


class Boost(Repository):
    def init(self):
        self.long_paths = True
        self.version_name = "boost_1_81_0"
        self.url = "https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz"
        self.address_model = ""
        self.modify_build_boost_arguments()
        self.name = "boost" + f"_x{self.address_model}" if self.address_model else ""

    def modify_build_boost_arguments(self):
        pass

    def build(self):
        boost_dir = THIRD_PARTY_PATH / self.version_name
        self.execute(
            f"./b2 address-model={self.address_model} install cxxflags=-fPIC runtime-link=static --with-container --with-regex --prefix=\"{str(THIRD_PARTY_PATH / self.name)}\" -j8 -d0 --hash",
            cwd=boost_dir,
        )

    def apply_patches(self):
        pass


class BoostWindows(Boost):
    def execute(self, cmd, cwd=THIRD_PARTY_PATH, must_pass=False):
        return super().execute(
            ["powershell", "-Command", cmd], cwd=cwd, must_pass=must_pass
        )

    def clone(self):
        self.execute(
            "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12"
        )
        self.execute(
            f"wget -Uri {self.url} -OutFile {self.name}.tar.gz -UserAgent [Microsoft.PowerShell.Commands.PSUserAgent]::InternetExplorer"
        )
        self.execute(f"tar xf {self.name}.tar.gz")
        self.execute("./bootstrap.bat", cwd=THIRD_PARTY_PATH / self.version_name)

    def cleanup(self):
        self.execute(f"rm -R {self.version_name} -Force")
        self.execute(f"rm {self.name}.tar.gz")


class BoostLinux(Boost):
    def execute(self, cmd, cwd=THIRD_PARTY_PATH):
        return super().execute(cmd, cwd, True)

    def clone(self):
        self.execute(f"wget {self.url} -O {self.name}.tar.gz")
        self.execute(f"tar xf {self.name}.tar.gz")
        self.execute("./bootstrap.sh", cwd=THIRD_PARTY_PATH / self.version_name)

    def cleanup(self):
        self.execute(f"rm -rf {self.version_name}")
        self.execute(f"rm {self.name}.tar.gz")


class BoostLinux64(BoostLinux):
    def modify_build_boost_arguments(self):
        self.address_model = "64"


class BoostLinux32(BoostLinux):
    def modify_build_boost_arguments(self):
        self.address_model = "32"


class BoostWindows64(BoostWindows):
    def modify_build_boost_arguments(self):
        self.address_model = "64"


class BoostWindows32(BoostWindows):
    def modify_build_boost_arguments(self):
        self.address_model = "32"


class BoostLinuxArm(BoostLinux):
    def modify_build_boost_arguments(self):
        raise Exception(
            "Automated building process of Boost on ARM is not currently supported."
        )


class Lua(Repository):
    def set_branch(self):
        self.branch = "v5.4.6"

    def init(self):
        self.name = "lua"
        self.url = "https://github.com/lua/lua.git"


class MurmurHash(Repository):
    def set_commit(self):
        self.commit_id = "61a0530f28277f2e850bfc39600ce61d02b518de"

    def init(self):
        self.name = "MurmurHash"
        self.url = "https://github.com/aappleby/smhasher.git"


class Zlib(Repository):
    def set_branch(self):
        self.branch = "v1.3"

    def init(self):
        self.name = "zlib"
        self.url = "https://github.com/madler/zlib.git"


class LibPng(Repository):
    def set_branch(self):
        self.branch = "v1.6.40"

    def init(self):
        self.name = "libpng"
        self.url = "https://github.com/glennrp/libpng.git"


class Xxhash(Repository):
    def set_branch(self):
        self.branch = "v0.8.1"

    def init(self):
        self.name = "xxhash"
        self.url = "https://github.com/Cyan4973/xxHash.git"


class StackWalker(Repository):
    def set_commit(self):
        self.commit_id = "045413aca44b094cf42cc85f8e62e986fe943697"

    def init(self):
        self.name = "StackWalker"
        self.url = "https://github.com/JochenKalmbach/StackWalker.git"
        self.os = OperatingSystem.WINDOWS


class ClHeaders(Repository):
    def set_branch(self):
        self.branch = "v2022.09.30"

    def init(self):
        self.name = "OpenCL-Headers"
        self.url = "https://github.com/KhronosGroup/OpenCL-Headers.git"


class RenderDoc(Repository):
    def set_branch(self):
        self.branch = "v1.21"

    def init(self):
        self.name = "renderdoc"
        self.url = "https://github.com/baldurk/renderdoc.git"


class Repositories:
    def __init__(self, args) -> None:
        self.repos = []
        if args.with_all or args.with_boost_64:
            if os.name == "nt":
                self.repos.append(BoostWindows64())
            else:
                self.repos.append(BoostLinux64())
        if args.with_boost_32:
            if os.name == "nt":
                self.repos.append(BoostWindows32())
            else:
                self.repos.append(BoostLinux32())
        if args.with_boost_arm:
            self.repos.append(BoostLinuxArm())
        if args.with_all or args.with_lua:
            self.repos.append(Lua())
        if args.with_all or args.with_murmurhash:
            self.repos.append(MurmurHash())
        if args.with_all or args.with_zlib:
            self.repos.append(Zlib())
        if args.with_all or args.with_libpng:
            self.repos.append(LibPng())
        if args.with_all or args.with_xxhash:
            self.repos.append(Xxhash())
        if args.with_all or args.with_stackwalker:
            self.repos.append(StackWalker())
        if args.with_all or args.with_clheaders:
            self.repos.append(ClHeaders())
        if args.with_all or args.with_renderdoc:
            self.repos.append(RenderDoc())

    def __iter__(self):
        for value in self.repos:
            yield value


def main(args=None):
    if not args:
        args = process_command_line()
    logger.debug("Arguments: %s", args)
    try:
        install_dependencies(args)
    except:
        logger.exception("Fatal error")
        return -1

    return 0


def process_command_line():
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    return parser.parse_args()


def setup_parser(root_parser):
    root_parser.add_argument("--with-all", action="store_true")
    root_parser.add_argument("--with-boost-32", action="store_true")
    root_parser.add_argument("--with-boost-64", action="store_true")
    root_parser.add_argument("--with-boost-arm", action="store_true")
    root_parser.add_argument("--with-lua", action="store_true")
    root_parser.add_argument("--with-murmurhash", action="store_true")
    root_parser.add_argument("--with-zlib", action="store_true")
    root_parser.add_argument("--with-libpng", action="store_true")
    root_parser.add_argument("--with-xxhash", action="store_true")
    root_parser.add_argument("--with-stackwalker", action="store_true")
    root_parser.add_argument("--with-clheaders", action="store_true")
    root_parser.add_argument("--with-renderdoc", action="store_true")


def install_dependencies(args):
    for repo in Repositories(args):
        repo.install()


if __name__ == "__main__":
    sys.exit(main())
