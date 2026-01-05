# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import argparse
import logging
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
import hashlib

from enum import Enum
from pathlib import Path
import shutil
import stat

import requests
from retrying import retry
import yaml


handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)-23s: %(levelname)-8s %(message)s')
handler.setFormatter(formatter)

logger = logging.getLogger('3rdParty')
logger.setLevel(logging.INFO)
if not logger.handlers:
    logger.addHandler(handler)
logger.propagate = False


def get_script_path():
    return os.path.dirname(os.path.realpath(sys.argv[0]))


def get_root_directory() -> Path:
    return Path(get_script_path()).parent


def get_file_hash(filename, algorithm='sha256'):
    hash_func = hashlib.new(algorithm)

    with open(filename, 'rb') as f:
        while chunk := f.read(8192):
            hash_func.update(chunk)

    return hash_func.hexdigest()


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
        min_major, min_minor, min_patch = self.get_versions(
            GIT_VERSION_PARALLEL)
        cur_major, cur_minor, cur_patch = self.get_versions(
            self.current_version)
        if cur_major > min_major:
            return True
        if cur_major == min_major and cur_minor > min_minor:
            return True
        if cur_major == min_major and cur_minor == min_minor and cur_patch >= min_patch:
            return True
        logger.info(f"Current GIT version {cur_major}.{cur_minor}.{cur_patch}")
        return False

THIRD_PARTY_FOLDER_NAME = "third_party"
THIRD_PARTY_PATH = get_root_directory() / THIRD_PARTY_FOLDER_NAME
DEPENDENCIES_FILE = THIRD_PARTY_PATH / "dependencies.yaml"
PATCH_PATH = THIRD_PARTY_PATH / "patch"
USE_PARALLEL_GIT = GitCheck().check()
META_FILE = "third_party.meta.yaml"


class OperatingSystem(Enum):
    WINDOWS = 1
    LINUX = 2

    @classmethod
    def from_str(cls, value: str):
        """Parse a string into an OperatingSystem or return None for unknown/empty."""
        if not value:
            return None
        v = value.upper()
        if v == "WINDOWS":
            return cls.WINDOWS
        if v == "LINUX":
            return cls.LINUX
        return None


class ReturnCode(Enum):
    SUCCESS = 0
    FAILURE = -1

    @classmethod
    def from_bool(cls, value: bool):
        if value:
            return cls.SUCCESS
        return cls.FAILURE


class RepoStatus(Enum):
    NOT_INSTALLED = 0
    WRONG_BASE = 1
    MISMATCHING_PATCHES = 2
    READY = 3

    def __str__(self):
        if RepoStatus.READY == self:
            return "ready"
        elif RepoStatus.MISMATCHING_PATCHES == self:
            return "mismatching patches"
        elif RepoStatus.WRONG_BASE == self:
            return "based on wrong commit/branch/url/path/..."
        elif RepoStatus.NOT_INSTALLED == self:
            return "not installed"
        else:
            return "unknown"


class RetryError(Exception):
    pass


class Dependency:
    def __init__(self, yaml_description) -> None:
        self._repository_path = None
        self.name = yaml_description["name"]
        self.argument = yaml_description.get("argument", self.name.lower())
        self.cli_argument = f"--with-{self.argument}"
        self.arg_name = self.cli_argument[2:].replace('-', '_')
        self.os = OperatingSystem.from_str(yaml_description.get("os", None))
        self.logger = logger.getChild(self.name)

    @property
    def repository_path(self) -> Path:
        if not self._repository_path:
            self._repository_path = THIRD_PARTY_PATH / self.name
            self.logger.debug(f"Component path: {self._repository_path}")
        return self._repository_path

    def relative_repository_path(self) -> Path:
        return self.repository_path.relative_to(THIRD_PARTY_PATH.parent)

    def check_skip_os(self) -> bool:
        return (self.os == OperatingSystem.WINDOWS and os.name != "nt") or (
            self.os == OperatingSystem.LINUX and os.name == "nt"
        )


    def prepare_folder(self, forced=False, accept_existing_folder=False):
        def remove_readonly(func, path, _):
            "Clear the readonly bit and reattempt the removal"
            os.chmod(path, stat.S_IWRITE)
            func(path)

        if self.repository_path.exists():
            self.logger.info(f"Existing dependency folder found @'{self.repository_path}'.")
            if accept_existing_folder:
                self.logger.info(f"Leaving it in place, assuming it's the correct git repo.")
                return True
            if not forced:
                self.logger.info(f"It's not registered and/or marked as wrongfully installed so the script will attempt to delete '{self.repository_path}'.")
            try:
                shutil.rmtree(self.repository_path, onexc=remove_readonly)
                self.logger.info(f"Deleted {self.repository_path}")
            except Exception as e:
                self.logger.error(f"Could not remove dependency at {self.repository_path}: {e}")
                self.logger.error("Please remove folder manually and try again.")
                return False

        self.repository_path.mkdir()
        return True

    def repository_path_exists_not_empty(self):
        return self.repository_path.exists() and any(self.repository_path.iterdir())

    def compare_and_log_difference(self, field, local, required):
        if local != required:
            self.logger.warning(f"Mismatch '{field}': local={local}, required={required}")
            return False
        return True

    def compare_and_log_difference_path(self, field, local, required):
        if local == required:
            return True

        local_parts = list(Path(local).parts)
        required_parts = list(Path(required).parts)

        min_len = min(len(local_parts), len(required_parts))
        paths_match = False

        for i in range(1, min_len + 1):
            local_part = local_parts[-i]
            if local_part != required_parts[-i]:
                break
            if local_part == THIRD_PARTY_FOLDER_NAME:
                paths_match = True
                break

        if not paths_match:
            self.logger.warning(f"Mismatch '{field}': local={local}, required={required}")
            return False
        return True


class GitRepository(Dependency):
    def __init__(self, yaml_description) -> None:
        super().__init__(yaml_description)
        self.url = yaml_description["url"]
        self.branch = yaml_description.get("branch", "master")
        self.commit_id = yaml_description.get("commit_id", "")
        self.sparse_checkout = yaml_description.get("sparse_checkout", "")
        self.modules = []
        self.patches = []
        if not THIRD_PARTY_PATH.exists():
            THIRD_PARTY_PATH.mkdir()

    def cleanup(self):
        if self.repository_path.exists():
            try:
                shutil.rmtree(self.repository_path)
            except (PermissionError, OSError) as e:
                self.logger.warning(f"Could not remove repository path {self.repository_path}: {e}")

    @retry(stop_max_attempt_number=5, wait_fixed=100000)
    def execute_with_retry(self, cmd, cwd=None, must_pass=True):
        self.logger.info(f"> {cmd}")
        process = subprocess.run(
            cmd,
            cwd=cwd if cwd else self.repository_path,
            shell=True,
            stderr=subprocess.PIPE,
        )
        if must_pass and process.returncode != 0:
            self.logger.error(process.stdout)
            self.logger.error(process.stderr)
        if must_pass:
            try:
                process.check_returncode()
            except subprocess.CalledProcessError as e:
                self.cleanup()
                self.logger.error(e.output)
                raise RetryError()
        return process

    def execute(self, cmd, cwd=None, must_pass=True):
        self.logger.info(f"> {cmd}")
        process = subprocess.run(
            cmd,
            cwd=cwd if cwd else self.repository_path,
            shell=True,
            stderr=subprocess.PIPE,
        )
        if must_pass and process.returncode != 0:
            self.logger.error(process.stdout)
            self.logger.error(process.stderr)
        if must_pass:
            try:
                process.check_returncode()
            except:
                exit(1)
        return process

    def apply_patches(self, repo_meta_info):
        self.patches = []
        patches_meta_info = repo_meta_info.get("patches", {})
        for i in self.modules:
            patch_list = list(Path(PATCH_PATH / i).glob("*.patch"))
            patch_hash_list = [(patch, get_file_hash(patch)) for patch in patch_list]
            hash_list = [hash for (patch, hash) in patch_hash_list]

            if set(hash_list) == set(patches_meta_info.get(i, [])):
                if len(hash_list) > 0:
                    self.logger.info(
                        f"Skipping patch application for '{i}' - already applied")
                    self.logger.info(
                        f"If patches are needed to be reapplied please rename the patch file temporarily.")
                continue

            self.logger.info(f"resetting repo {i}")
            self.execute(
                f"git reset --hard",
                cwd=self.repository_path.parent / i,
                must_pass=True
            )
            self.execute(
                f"git clean -fdx",
                cwd=self.repository_path.parent / i,
                must_pass=True
            )
            patches_meta_info[str(i)] = []
            if len(hash_list) == 0:
                continue
            for (patch, hash) in patch_hash_list:
                self.patches.append(patch)
                self.execute(
                    f"git apply {patch} --ignore-whitespace", cwd=self.repository_path.parent / i
                )
                patches_meta_info[str(i)].append(str(hash))
        repo_meta_info["patches"] = patches_meta_info
        return repo_meta_info

    def clone(self):
        try:
            clone_cmd = [
                f"git clone {self.url} --depth 1 --branch {self.branch} --recursive {self.name}"
            ]
            if USE_PARALLEL_GIT:
                clone_cmd.append("-j 8")
            if self.sparse_checkout:
                clone_cmd.append("--filter=blob:none --no-checkout")
            self.execute_with_retry(
                " ".join(clone_cmd),
                cwd=self.repository_path.parent,
            )
            if self.sparse_checkout:
                self.execute(f"git sparse-checkout set --no-cone {self.sparse_checkout}")
                self.execute(f"git checkout {self.branch}")
            if self.commit_id:
                self.execute(f"git fetch --depth 1 origin {self.commit_id}")
                self.execute(f"git checkout {self.commit_id}")
        except Exception as e:
            self.logger.error(f"Error during clone: {e}")
            return False
        return True

    def scan_repositories(self, do_clean=False) -> None:
        if not self.repository_path.exists():
            return

        modules_paths = self.repository_path.rglob(".git")
        for submodule_path in modules_paths:
            path = submodule_path.relative_to(self.repository_path.parent).parent
            if path not in self.modules:
                self.modules.append(path)
            if do_clean:
                self.execute(
                    f"git reset --hard",
                    cwd=self.repository_path.parent / path,
                    must_pass=True
                )
                self.execute(
                    f"git clean -fdx",
                    cwd=self.repository_path.parent / path,
                    must_pass=True
                )

    def last_install_sufficient(self, repo_meta_info):
        if repo_meta_info.get("status", None) != "installed":
            return RepoStatus.NOT_INSTALLED

        result = True and self.compare_and_log_difference_path("path", repo_meta_info.get("path", None), str(self.repository_path))
        result = result and self.compare_and_log_difference("branch", repo_meta_info.get("branch", None), self.branch)
        result = result and self.compare_and_log_difference("commit id", repo_meta_info.get("commit_id", None), self.commit_id)
        result = result and self.compare_and_log_difference("URL", repo_meta_info.get("url", None), self.url)

        if not result:
            return RepoStatus.WRONG_BASE

        patches_meta_info = repo_meta_info.get("patches", {})
        self.patches = []
        for path_name in self.modules:
            patch_list = list(Path(PATCH_PATH / path_name).glob("*.patch"))
            patch_hash_list = [(patch, get_file_hash(patch)) for patch in patch_list]
            hash_list = [hash for (patch, hash) in patch_hash_list]
            module_name = str(path_name)
            module_patches = set(patches_meta_info.get(module_name, []))
            if set(hash_list) != module_patches:
                self.logger.warning(f"Mismatch 'patches' for module '{module_name}': local={module_patches}, required={hash_list}")
                result = False
            else:
                self.patches.extend(patch_list)
        if not result:
            return RepoStatus.MISMATCHING_PATCHES

        return RepoStatus.READY

    def install(self, repo_meta_info, offline_mode):
        if self.check_skip_os():
            self.logger.debug("Skipping installation due to OS limitation")
            repo_meta_info["status"] = "skipped"
            return True, repo_meta_info

        self.scan_repositories()

        repo_status = self.last_install_sufficient(repo_meta_info)

        if RepoStatus.READY == repo_status:
            if self.repository_path_exists_not_empty():
                msg = f"Found repository ready @ {self.relative_repository_path()}"
                patch_count = len(self.patches)
                if patch_count > 0:
                    msg += f" - {patch_count} patch(es) applied"
                self.logger.info(msg)
                return True, repo_meta_info
        else:
            self.logger.info(f"Last install status is {repo_status}")

        if RepoStatus.WRONG_BASE == repo_status and offline_mode:
            self.logger.error(f"Wrong version installed @ {self.relative_repository_path()}")
            self.logger.error(f"Details: {repo_meta_info}")
            self.logger.error(f"Offline mode can not change the installed version!")
            return False, repo_meta_info

        repo_meta_info["url"] = self.url
        repo_meta_info["commit_id"] = self.commit_id
        repo_meta_info["branch"] = self.branch
        repo_meta_info["path"] = str(self.repository_path)
        repo_meta_info["status"] = "required"

        needs_install = RepoStatus.WRONG_BASE == repo_status or RepoStatus.NOT_INSTALLED == repo_status
        if needs_install and not offline_mode:
            if not self.prepare_folder():
                return False, repo_meta_info
            repo_meta_info["status"] = "folder_prepared"
            if not self.clone():
                return False, repo_meta_info
            repo_meta_info["status"] = "cloned"

        if not self.repository_path_exists_not_empty():
            if not offline_mode:
                self.logger.error(f"The folder doesn't exist or is empty after the clone @ {self.repository_path}")
            else:
                if not self.repository_path.exists():
                    self.logger.error(f"The prepared folder doesn't exist @ {self.repository_path}")
                else:
                    self.logger.error(f"The prepared folder is empty: {self.repository_path}")
            repo_meta_info["status"] = "clone_failure"
            return False, repo_meta_info
        else:
            if offline_mode:
                self.logger.info(f"Found prepared repository @ {self.relative_repository_path()}")

        self.scan_repositories(RepoStatus.MISMATCHING_PATCHES == repo_status)
        repo_meta_info = self.apply_patches(repo_meta_info)
        repo_meta_info["status"] = "installed"

        msg = f"Repository ready @ {self.relative_repository_path()}"
        patch_count = len(self.patches)
        if patch_count > 0:
            msg += f" - {patch_count} patch(es) applied"
        self.logger.info(msg)
        return True, repo_meta_info


class Nuget(Dependency):
    def __init__(self, yaml_description) -> None:
        super().__init__(yaml_description)
        self.package_url = yaml_description["package_url"]
        self.version = yaml_description["version"]
        self.folder_to_extract = yaml_description.get("folder_to_extract", '')
        if self.folder_to_extract is None:
            self.folder_to_extract = ''

    def last_install_sufficient(self, repo_meta_info):
        if repo_meta_info.get("status", None) != "installed":
            return False

        result = True and self.compare_and_log_difference("Package URL", repo_meta_info.get("package_url", None), self.package_url)
        result = result and self.compare_and_log_difference("version", repo_meta_info.get("version", None), self.version)
        result = result and self.compare_and_log_difference("Folder to extract", repo_meta_info.get("folder_to_extract", ''), str(self.folder_to_extract))

        return result

    def install(self, repo_meta_info, offline_mode):
        if repo_meta_info.get("folder_to_extract", None) is None:
            repo_meta_info["folder_to_extract"] = ''

        if self.check_skip_os():
            self.logger.debug("Skipping installation due to OS limitation")
            repo_meta_info["status"] = "skipped"
            return True, repo_meta_info

        if self.last_install_sufficient(repo_meta_info):
            if self.repository_path_exists_not_empty():
                self.logger.info(f"Nuget Package ready @ {self.relative_repository_path()}")
                return True, repo_meta_info
        else:
            if offline_mode and repo_meta_info.get("status", None) == "installed":
                self.logger.error(f"Wrong version installed @ {self.relative_repository_path()}")
                self.logger.error(f"Details: {repo_meta_info}")
                self.logger.error(f"Offline mode can not change the installed version!")
                return False, repo_meta_info

        repo_meta_info["version"] = self.version
        repo_meta_info["package_url"] = self.package_url
        repo_meta_info["folder_to_extract"] = self.folder_to_extract
        repo_meta_info["path"] = str(self.repository_path)
        repo_meta_info["status"] = "failed"

        if not offline_mode:
            if not self.prepare_folder():
                return False, repo_meta_info
            repo_meta_info["status"] = "folder_prepared"

            self.logger.info(
                f"Starting download and extraction of package @ {self.relative_repository_path()}")
            self.download_and_extract_nuget_package(
                self.package_url + self.version, self.repository_path)
            self.logger.info(
                "Download and extraction completed for package.")

        repo_meta_info["status"] = "extracted"
        if not self.repository_path_exists_not_empty():
            if not offline_mode:
                self.logger.error(f"The folder doesn't exist or is empty after extraction @ {self.repository_path}")
            else:
                if not self.repository_path.exists():
                    self.logger.error(f"The prepared folder doesn't exist @ {self.repository_path}")
                else:
                    self.logger.error(f"The prepared folder is empty: {self.repository_path}")
            repo_meta_info["status"] = "extraction_failure"
            return False, repo_meta_info
        else:
            if offline_mode:
                self.logger.info(f"Found prepared Nuget package @ {self.relative_repository_path()}")
        repo_meta_info["status"] = "installed"

        return True, repo_meta_info


    @retry(stop_max_attempt_number=5, wait_fixed=100000)
    def download_and_extract_nuget_package(self, package_url, output_directory: Path):
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
            self.logger.info(f"Using HTTP proxy: {http_proxy}")
        if https_proxy:
            proxies["https"] = https_proxy
            self.logger.info(f"Using HTTPS proxy: {https_proxy}")
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
            if self.folder_to_extract:
                with tempfile.TemporaryDirectory() as temp_dir:
                    for member in zip_ref.namelist():
                        # Extract files from the main directory (e.g., licenses, README, etc.)
                        if '/' not in member.strip('/'):
                            zip_ref.extract(member, output_directory)
                        # Extract files and directories from the specified subdirectory
                        elif member.startswith(self.folder_to_extract):
                            zip_ref.extract(member, temp_dir)
                    # Move the contents from the temporary directory to the output directory
                    temp_dir_path = Path(temp_dir)
                    for item in (temp_dir_path / self.folder_to_extract).iterdir():
                        shutil.move(temp_dir_path / self.folder_to_extract /
                                    item, output_directory)
            else:
                zip_ref.extractall(output_directory)

        # Delete the downloaded package file
        os.remove(package_path)


def main(arguments=None) -> int:
    result = True
    dependencies = []
    try:
        dependencies = read_dependencies()
    except:
        logger.exception("Fatal error")
        return ReturnCode.FAILURE.value

    args = process_command_line(dependencies, arguments)

    logger.debug(f"Arguments: {args}")

    meta_data = read_meta_data(args.meta_path / META_FILE)
    try:
        result = install_dependencies(args, dependencies, meta_data)
    except:
        result = False
        logger.exception("Fatal error")
    finally:
        write_meta_data(args, META_FILE, meta_data)

    return ReturnCode.from_bool(result).value


def read_dependencies():
    with open(DEPENDENCIES_FILE, "r") as f:
        dependencies = yaml.safe_load(f)["dependencies"]
    return [Nuget(dep_nuget) for dep_nuget in dependencies.get("NugetPackages", [])] + [GitRepository(dep_git_repo) for dep_git_repo in dependencies.get("GitRepositories", [])]


def process_command_line(dependencies, arguments=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser, dependencies)
    return parser.parse_args(arguments)


def setup_parser(root_parser, dependencies):
    for dependency in dependencies:
        root_parser.add_argument(
            dependency.cli_argument, action="store_true", help=f"Install {dependency.name}")
    root_parser.add_argument(
        "--with-all", action="store_true", help="Install all dependencies")
    root_parser.add_argument(
        "--meta-path", type=Path, help="Path to the directory holding the meta file", default=str(THIRD_PARTY_PATH))
    root_parser.add_argument(
        "--offline-mode", action="store_true", help="Assume encountered folders contain the repository in the correct unpatched version/branch/commit id.")


def read_meta_data(meta_file_path):
    meta_data = {}
    try:
        if meta_file_path.exists():
            with open(meta_file_path, "r") as f:
                meta_data = yaml.safe_load(f)
            if meta_data is None:
                meta_data = {}
        else:
            logger.info("No meta data found")
    except Exception as e:
        logger.error(f"Failed to read meta data: {e}")
    return meta_data


def install_dependencies(args, dependencies, meta_data) -> bool:
    result = True
    for dependency in dependencies:
        if args.with_all or getattr(args, dependency.arg_name, False):
            result_dep = False
            try:
                repo_meta_info = meta_data.get(dependency.name, {})
                result_dep, repo_meta_info = dependency.install(repo_meta_info, offline_mode=args.offline_mode)
                meta_data[dependency.name] = repo_meta_info
            except Exception as e:
                dependency.logger.exception(f"Encountered exception: {e}")
                result_dep = False
            if not result_dep:
                logger.exception(f"Failed installing dependency {dependency.name} to {dependency.relative_repository_path()}")
            result = result or result_dep
    return result


def write_meta_data(args, meta_file, meta_data):
    meta_file_path = args.meta_path /  meta_file
    try:
        with open(meta_file_path, "w") as f:
            yaml.dump(meta_data, f)
    except Exception as e:
        logger.error(f"Failed to write meta data: {e}")


if __name__ == "__main__":
    sys.exit(main())
