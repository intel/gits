# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
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
import requests
import zipfile
import tempfile
from retrying import retry
import shutil
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


class RetryError(Exception):
    pass


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
        if self.repository_path.exists():
            shutil.rmtree(self.repository_path)

    @retry(stop_max_attempt_number=5, wait_fixed=100000)
    def execute_with_retry(self, cmd, cwd=None, must_pass=True):
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
            except subprocess.CalledProcessError as e:
                self.cleanup()
                logger.error(e.output)
                raise RetryError()
        return process

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
                f"git clone {self.url} --depth 1 --branch {self.branch} --recursive {self.name}"
            ]
            if USE_PARALLEL_GIT:
                clone_cmd.append("-j 8")
            if self.long_paths:
                clone_cmd.append("-c core.longpaths=true")
            self.execute_with_retry(
                " ".join(clone_cmd),
                cwd=self.repository_path.parent,
            )
            if self.commit_id:
                self.execute(f"git fetch --depth 1 origin {self.commit_id}")
                self.execute(f"git checkout {self.commit_id}")

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
            self.scan_repositories()
            self.apply_patches()
            if install:
                self.build()
        except subprocess.CalledProcessError:
            logger.exception("Issue installing repository.")
            return -1
        return 0


@retry(stop_max_attempt_number=5, wait_fixed=100000)
def download_and_extract_nuget_package(package_url, output_directory: Path, folder_to_extract=None):
    # Ensure the output directory exists
    output_directory.mkdir(parents=True, exist_ok=True)

    # Retrieve proxy settings from Git
    try:
        http_proxy = subprocess.run(
            "git config --global --get http.proxy",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
            shell=True
        ).stdout.decode().strip()
    except subprocess.CalledProcessError:
        http_proxy = None

    try:
        https_proxy = subprocess.run(
            "git config --global --get https.proxy",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
            shell=True
        ).stdout.decode().strip()
    except subprocess.CalledProcessError:
        https_proxy = None

    # Set up the requests session with proxy settings
    session = requests.Session()
    proxies = {}
    if http_proxy:
        proxies["http"] = http_proxy
        proxies["https"] = http_proxy
        logger.info(f"Using HTTP proxy: {http_proxy}")
    if https_proxy:
        proxies["https"] = https_proxy
        logger.info(f"Using HTTPS proxy: {https_proxy}")
    if proxies:
        session.proxies = proxies

    # Download the package
    response = session.get(package_url)
    package_path = output_directory / "package.nupkg"

    # Save the package to the output directory
    with open(package_path, "wb") as file:
        file.write(response.content)

    # Extract the package
    with zipfile.ZipFile(package_path, 'r') as zip_ref:
        if folder_to_extract:
            with tempfile.TemporaryDirectory() as temp_dir:
                for member in zip_ref.namelist():
                    # Extract files from the main directory (e.g., licenses, README, etc.)
                    if '/' not in member.strip('/'):
                        zip_ref.extract(member, output_directory)
                    # Extract files and directories from the specified subdirectory
                    elif member.startswith(folder_to_extract):
                        zip_ref.extract(member, temp_dir)
                # Move the contents from the temporary directory to the output directory
                temp_dir_path = Path(temp_dir)
                for item in (temp_dir_path / folder_to_extract).iterdir():
                     shutil.move(temp_dir_path / folder_to_extract / item, output_directory)
        else:
            zip_ref.extractall(output_directory)

    # Delete the downloaded package file
    os.remove(package_path)


class Nuget(Repository):
    def install(self):
        try:
            if self.check_skip_os():
                logger.debug("Skipping %s installation due to os limitation", self.name)
                return 0
            install = True
            if self.repository_path.exists() and any(self.repository_path.iterdir()):
                logger.info(f"Package is already downloaded and extracted: {self.name}")
                install = False
            if install:
                logger.info(f"Starting download and extraction of package: {self.name}")
                download_and_extract_nuget_package(self.package_url + self.version, self.repository_path, self.folder_to_extract)
                logger.info(f"Download and extraction completed for package: {self.name}")
                # Check if the directory is empty after extraction
                if not any(self.repository_path.iterdir()):
                    raise Exception(f"Extraction failed, {self.repository_path} is empty.")
        except Exception as e:
            logger.exception("Issue installing nuget package.")
            return -1
        return 0


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
        self.branch = "v1.3.1"

    def init(self):
        self.name = "zlib"
        self.url = "https://github.com/madler/zlib.git"


class LibPng(Repository):
    def set_branch(self):
        self.branch = "v1.6.43"

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


class Detours(Repository):
    def set_branch(self):
        self.branch = "main"

    def set_commit(self):
        self.commit_id = "4b8c659f549b0ab21cf649377c7a84eb708f5e68"

    def init(self):
        self.name = "Detours"
        self.url = "https://github.com/microsoft/Detours.git"
        self.os = OperatingSystem.WINDOWS


class DirectXTex(Repository):
    def set_branch(self):
        self.branch = "sep2024"

    def init(self):
        self.name = "DirectXTex"
        self.url = "https://github.com/microsoft/DirectXTex.git"
        self.os = OperatingSystem.WINDOWS


class XeSS(Repository):
    def set_branch(self):
        self.branch = "v2.0.1"

    def init(self):
        self.name = "xess"
        self.url = "https://github.com/intel/xess.git"
        self.os = OperatingSystem.WINDOWS


class AgilitySDK(Nuget):
    def init(self):
        self.name = "AgilitySDK"
        self.package_url = "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/"
        self.folder_to_extract = "build/native/"
        self.version = "1.614.1"
        self.os = OperatingSystem.WINDOWS


class DXC(Nuget):
    def init(self):
        self.name = "dxc"
        self.package_url = "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DXC/"
        self.folder_to_extract = "build/native/"
        self.version = "1.8.2407.12"
        self.os = OperatingSystem.WINDOWS


class DirectML(Nuget):
    def init(self):
        self.name = "DirectML"
        self.package_url = "https://www.nuget.org/api/v2/package/Microsoft.AI.DirectML/"
        self.folder_to_extract = None
        self.version = "1.15.2"
        self.os = OperatingSystem.WINDOWS


class DirectStorage(Nuget):
    def init(self):
        self.name = "DirectStorage"
        self.package_url = "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DirectStorage/"
        self.folder_to_extract = None
        self.version = "1.2.3"
        self.os = OperatingSystem.WINDOWS


class fastio(Repository):
    def set_commit(self):
        self.commit_id = "788cb9810f0ca881d15fe594b0dd1144901d14d8"

    def init(self):
        self.name = "fast_io"
        self.url = "https://github.com/cppfastio/fast_io.git"


class RenderDoc(Repository):
    def set_branch(self):
        self.branch = "v1.21"

    def init(self):
        self.name = "renderdoc"
        self.url = "https://github.com/baldurk/renderdoc.git"


class NlohmannJson(Repository):
    def set_branch(self):
        self.branch = "v3.11.3"

    def init(self):
        self.name = "json"
        self.url = "https://github.com/nlohmann/json.git"


class lz4(Repository):
    def set_branch(self):
        self.branch = "v1.10.0"

    def init(self):
        self.name = "lz4"
        self.url = "https://github.com/lz4/lz4.git"


class zstd(Repository):
    def set_branch(self):
        self.branch = "v1.5.6"

    def init(self):
        self.name = "zstd"
        self.url = "https://github.com/facebook/zstd.git"


class YamlCPP(Repository):
    def set_branch(self):
        self.branch = "0.8.0"

    def init(self):
        self.name = "yaml-cpp"
        self.url = "https://github.com/jbeder/yaml-cpp.git"


class ArgsHXX(Repository):
    def set_commit(self):
        self.commit_id = "114200a9ad5fe06c8dea76e15d92325695cf3e34"

    def init(self):
        self.name = "argshxx"
        self.url = "https://github.com/Taywee/args.git"


class Repositories:
    def __init__(self, args) -> None:
        self.repos = []
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
        if args.with_all or args.with_detours:
            self.repos.append(Detours())
        if args.with_all or args.with_directxtex:
            self.repos.append(DirectXTex())
        if args.with_all or args.with_xess:
            self.repos.append(XeSS())
        if args.with_all or args.with_agility_sdk:
            self.repos.append(AgilitySDK())
        if args.with_all or args.with_dxc:
            self.repos.append(DXC())
        if args.with_all or args.with_directml:
            self.repos.append(DirectML())
        if args.with_all or args.with_directstorage:
            self.repos.append(DirectStorage())
        if args.with_all or args.with_fastio:
            self.repos.append(fastio())
        if args.with_all or args.with_renderdoc:
            self.repos.append(RenderDoc())
        if args.with_all or args.with_json:
            self.repos.append(NlohmannJson())
        if args.with_all or args.with_lz4:
            self.repos.append(lz4())
        if args.with_all or args.with_zstd:
            self.repos.append(zstd())
        if args.with_all or args.with_yamlcpp:
            self.repos.append(YamlCPP())
        if args.with_all or args.with_argshxx:
            self.repos.append(ArgsHXX())

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
    root_parser.add_argument("--with-lua", action="store_true")
    root_parser.add_argument("--with-murmurhash", action="store_true")
    root_parser.add_argument("--with-zlib", action="store_true")
    root_parser.add_argument("--with-libpng", action="store_true")
    root_parser.add_argument("--with-xxhash", action="store_true")
    root_parser.add_argument("--with-stackwalker", action="store_true")
    root_parser.add_argument("--with-clheaders", action="store_true")
    root_parser.add_argument("--with-lz4", action="store_true")
    root_parser.add_argument("--with-zstd", action="store_true")
    root_parser.add_argument("--with-detours", action="store_true")
    root_parser.add_argument("--with-directxtex", action="store_true")
    root_parser.add_argument("--with-xess", action="store_true")
    root_parser.add_argument("--with-agility-sdk", action="store_true")
    root_parser.add_argument("--with-dxc", action="store_true")
    root_parser.add_argument("--with-directml", action="store_true")
    root_parser.add_argument("--with-directstorage", action="store_true")
    root_parser.add_argument("--with-fastio", action="store_true")
    root_parser.add_argument("--with-renderdoc", action="store_true")
    root_parser.add_argument("--with-json", action="store_true")
    root_parser.add_argument("--with-yamlcpp", action="store_true")
    root_parser.add_argument("--with-argshxx", action="store_true")


def install_dependencies(args):
    for repo in Repositories(args):
        repo.install()


if __name__ == "__main__":
    sys.exit(main())
