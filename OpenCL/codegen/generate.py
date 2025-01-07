#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import copy
import os
import sys
import re
from typing import Any

import mako.template
import mako.exceptions

import generator_cl

blocklist = ("nanf",)

callbacks = {
    "CallbackProgram": "void (CL_CALLBACK *{name})(cl_program, void*)",
    "CallbackContext": "void (CL_CALLBACK *{name})(const char*, const void*, size_t, void*)",
    "CallbackEvent": "void (CL_CALLBACK *{name})(cl_event, cl_int, void*)",
    "CallbackMem": "void (CL_CALLBACK *{name})(cl_mem, void*)",
    "CallbackFunc": "void (CL_CALLBACK *{name})(void*)",
    "CallbackSVM": "void (CL_CALLBACK *{name})(cl_command_queue, cl_uint, void**, void*)",
}


def _make_id(name: str) -> str:
    id_ = re.sub("([a-z])([A-Z])", r"\g<1>_\g<2>", name)
    id_ = re.sub("(D3D[0-9]+)([A-Z])", r"\g<1>_\g<2>", id_)
    id_ = re.sub("(DX[0-9])([A-Z])", r"\g<1>_\g<2>", id_)
    id_ = re.sub("([^_])(KHR|EXT|INTEL|NV)", r"\g<1>_\g<2>", id_)

    id_final = "ID_" + id_.upper().strip("_")
    return id_final


def _is_type(object: dict, type: Any, strict=False) -> bool:
    def compare(op1, op2):
        return op1 == op2 if strict else op1 in op2

    if isinstance(type, str):
        return compare(type, object["wrapType"]) or compare(type, object["type"])
    elif isinstance(type, (tuple, list)):
        return any([compare(t, object["wrapType"]) or compare(t, object["type"]) for t in type])


def _is_zero_ended(argument: dict) -> bool:
    # cl_command_queue_properties does not represent
    # a zero ended array of properties, it's an enum
    return _is_type(argument, "const char*", strict=True) or (
        is_pointer(argument)
        and _is_type(argument, ("properties", "property"))
        and _extract_type(argument["type"]) != "cl_command_queue_properties"
    )


def _extract_type(type: str) -> str:
    return type.replace("const ", "").replace("*", "")


def _process_functions(functions: dict) -> None:
    for blocklisted in blocklist:
        functions.pop(blocklisted, None)

    for name, func in functions.items():
        functions[name]["version"] = func.get("version", 0)
        functions[name]["recExecWrapName"] = name + "_RECEXECWRAP"
        functions[name]["runWrapName"] = name + "_RUNWRAP"
        functions[name]["stateTrackName"] = cut_version(name, func["version"]) + "_SD"
        functions[name]["recWrapName"] = name + "_RECWRAP"
        functions[name]["id"] = _make_id(name)
        functions[name]["availableFrom"] = _make_version(func)

    # process inheritance
    for name, func in functions.items():
        if func.get("inheritFrom"):
            # preserve properties
            extension = ""
            enabled = functions[name]["enabled"]
            version = functions[name]["availableFrom"]
            idBuffer = functions[name]["id"]
            recWrapName = functions[name]["recWrapName"]
            if functions[name].get("extension"):
                extension = functions[name]["extension"]
            functions[name] = copy.deepcopy(functions[func["inheritFrom"]])
            functions[name]["enabled"] = enabled
            functions[name]["availableFrom"] = version
            functions[name]["extension"] = extension
            functions[name]["id"] = idBuffer
            functions[name]["recWrapName"] = recWrapName

        # this step must be performed after inheritance has been processed
        # check if these changes were not already done
        if not functions[name].get("functionType"):
            functions[name]["functionType"] = functions[name]["type"]
            functions[name]["type"] = functions[name]["retV"]["type"]
            functions[name]["wrapType"] = functions[name]["retV"]["type"]
            if functions[name]["retV"].get("wrapType"):
                functions[name]["wrapType"] = functions[name]["retV"]["wrapType"]
            if functions[name]["retV"].get("wrapParams"):
                functions[name]["wrapParams"] = functions[name]["retV"]["wrapParams"]

        if not functions[name].get("args"):
            functions[name]["args"] = list()
            args = len([s for s in functions[name].keys() if s.startswith("arg")])
            for idx in range(1, args):
                idx = "arg" + str(idx)
                if not functions[name][idx].get("wrapType"):
                    functions[name][idx]["wrapType"] = functions[name][idx]["type"]
                functions[name]["args"].append(functions[name][idx])
                del functions[name][idx]


def _process_enums(enums: dict) -> None:
    for name in enums.keys():
        if not enums[name].get("vars"):
            args = max([int(s.replace("var", "")) for s in enums[name].keys() if s.startswith("var")]) + 1
            enums[name]["vars"] = list()
            for idx in range(1, args):
                idx = "var" + str(idx)
                if enums[name].get(idx):
                    enums[name]["vars"].append(enums[name][idx])
                    del enums[name][idx]


def _process_arguments(arguments: dict) -> None:
    for name in arguments.keys():
        if arguments[name].get("obj"):
            arguments[name]["mutual_mapping"] = []
        arguments[name]["version"] = arguments[name].get("version", 0)
        if arguments[name].get("alias"):
            arguments[arguments[name].get("alias")]["mutual_mapping"].append(name)
            arguments[name]["mutual_mapping"].append(arguments[name].get("alias"))
        if not arguments[name].get("vars"):
            vars = [int(s.replace("var", "")) for s in arguments[name].keys() if s.startswith("var")]
            if vars:
                arguments[name]["vars"] = list()
                for idx in range(1, max(vars) + 1):
                    idx = "var" + str(idx)
                    if arguments[name].get(idx):
                        arguments[name]["vars"].append(arguments[name][idx])
                        del arguments[name][idx]


def _prepare_dev_files(dir: str, dev_files: dict) -> None:
    for proj in dev_files:
        for f in dev_files[proj]:
            dev_file_path = os.path.join(dir, proj, f)
            if f.endswith(".h"):
                dev_file_path = os.path.join(dir, proj, "include", f)
            source_read = open(dev_file_path, "r").read()
            source_read = source_read.replace("\n} // namespace OpenCL\n} // namespace gits", "")
            with open(dev_file_path, "w") as source_write:
                source_write.write(source_read)


def _make_version(function: dict) -> str:
    return function["availableFrom"].replace(".", "_")


def _expand_callback(arg: dict) -> str:
    return callbacks[arg["type"]].format(name=arg["name"])


def _get_arg(arg: dict, with_types: bool) -> str:
    if with_types and "Callback" in arg["type"]:
        return _expand_callback(arg)
    return (f'{arg["type"]} ' if with_types else "") + (arg["name"])


def cut_version(name: str, version: int) -> str:
    return name.replace(f"_V{str(version)}", "")


def latest_version(objects: dict) -> dict:
    latest_items = copy.deepcopy(objects)
    for name in objects:
        if (name + "_V" + str(objects[name].get("version", 0) + 1)) in objects.keys():
            del latest_items[name]
    return latest_items


def only_enabled(objects: dict) -> dict:
    enabled_items = copy.deepcopy(objects)
    for name in objects:
        if not objects[name]["enabled"]:
            del enabled_items[name]
    return enabled_items


def without_field(objects: dict, field: str) -> dict:
    without_field_items = copy.deepcopy(objects)
    for name in objects:
        if objects[name].get(field):
            del without_field_items[name]
    return without_field_items


def is_pointer(func: dict) -> bool:
    return _is_type(func, "*") and _is_type(func, ("cl", "size_t"))


def make_params(
    func: dict, prefix=str(), with_retval=False, with_types=False, prepend_comma=False, one_line=False, tabs_num=1
) -> str:
    contain_args = len(func["args"]) > 0
    seperator = ", " if one_line else ","
    str_params = seperator if prepend_comma and contain_args else ""
    new_line = f'\n{"  " * tabs_num}' if not one_line else ""
    if with_retval and func.get("type") != "void":
        str_params += new_line + prefix
        if with_types:
            str_params += f'{func["type"]} '
        str_params += "return_value"
    if contain_args:
        if with_retval and func.get("type") != "void":
            str_params += seperator
        str_params += seperator.join([new_line + prefix + _get_arg(arg, with_types) for arg in func["args"]])
    return str_params


def get_wrap_type(argument: dict) -> str:
    type = argument["type"]
    if argument.get("wrapType") and argument.get("wrapType") != type:
        return argument["wrapType"]
    raw_type = _extract_type(type)
    if type == "void*":
        return "CCLMappedPtr"
    if "callbacks" in type or "void" in type:
        return "CvoidPtr"
    elif "cl_int" in type:
        raw_type = "CLResult"
    if "*" in type and not argument.get("custom"):
        return f"C{raw_type}::CSArray"
    return f"C{raw_type}"


def get_return_type(func: dict) -> str:
    return "void_t" if func.get("type") == "void" else func.get("type")


def format_trace_argument(arg: dict, enums: dict) -> str:
    if not arg:
        return ""

    def _get_tracing_wrapper_type(argument: dict) -> str:
        if argument["name"] == "errcode_ret":
            return "TCLResult"
        arg_type = _extract_type(argument["type"])
        if arg_type in enums.keys():
            return "T" + arg_type
        return arg_type

    def _get_tracing_params(argument: dict) -> str:
        params = "{name}"
        if (
            argument.get("wrapParams")
            and (argument["wrapParams"].count(",") == 1 or _is_zero_ended(argument))
            and not argument["name"] == "lengths"
            and not argument["name"] == "mapped_ptr"
        ):
            # if there are exactly two arguments in wrapParams and it's not
            # lengths and mapped_ptr
            params = argument.get("wrapParams")
        else:
            params = "1, {name}"
        return params.format(name=argument["name"])

    def _argument(argument: dict) -> dict:
        if (
            is_pointer(argument)
            or "void*" in argument["type"]
            and not argument.get("wrapType")
            in ["CAsyncBinaryData", "CSVMPtr", "CUSMPtr", "CUSMAreaPtr", "CCLMappedPtr", "CSVMPtr_V1"]
        ):
            array = f'TOclArray<{_extract_type(argument["type"])}, {_get_tracing_wrapper_type(argument)}>'
            return f"{array}({_get_tracing_params(argument)})"
        else:
            if argument["type"] in enums.keys():
                return f'T{argument["type"]}({argument["name"]})'
        return argument["name"]

    return _argument(arg)


def _mako_write(inpath: str, outpath: str, **args: dict) -> int:
    try:
        template = mako.template.Template(filename=inpath)
        rendered = template.render(
            make_params=make_params,
            get_wrap_type=get_wrap_type,
            latest_version=latest_version,
            only_enabled=only_enabled,
            without_field=without_field,
            cut_version=cut_version,
            format_trace_argument=format_trace_argument,
            get_return_type=get_return_type,
            is_pointer=is_pointer,
            **args,
        )
        rendered = re.sub(r"\r\n", r"\n", rendered)

        with open(outpath, "w") as fout:
            fout.write(rendered)
    except:
        traceback = mako.exceptions.RichTraceback()
        for (filename, lineno, function, line) in traceback.traceback:
            print(f"{filename}({lineno}) : error in {function}")
            print(line, "\n")
        print(f"{str(traceback.error.__class__.__name__)}: {traceback.error}")
        return -1
    return 0


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: {} outpath".format(sys.argv[0]))
        sys.exit(1)

    pathin = ""
    pathout = sys.argv[1]

    functions = generator_cl.GetFunctions()
    _process_functions(functions)
    functions = dict(sorted(functions.items()))
    enums = generator_cl.GetEnums()
    _process_enums(enums)
    enums = dict(sorted(enums.items()))
    arguments = generator_cl.GetArguments()
    _process_arguments(arguments)
    arguments = dict(sorted(arguments.items()))

    if sys.argv[1] == "update":
        dir = os.path.dirname(os.path.dirname(__file__))
        dev_files = {}
        dev_files["common"] = ["openclPlayerRunWrap.h", "openclStateTracking.h"]
        dev_files["recorder"] = ["openclRecorderSubwrappers.h"]
        dev_files["interceptor"] = ["openclExecWrap.h"]
        _prepare_dev_files(dir, dev_files)
        for proj in dev_files:
            for f in dev_files[proj]:
                dev_path = os.path.join(dir, proj, f)
                if f.endswith(".h"):
                    dev_path = os.path.join(dir, proj, "include", f)
                print(f"Updating {dev_path}...")
                ret_val = _mako_write(
                    os.path.join(dir, "codegen/templates", f + ".mako"),
                    dev_path,
                    output_path=dev_path,
                    functions=functions,
                    enums=enums,
                    arguments=arguments,
                )
                if ret_val != 0:
                    sys.exit(ret_val)
        sys.exit(0)

    print(f'Generating {os.path.join(pathout, "openclIDs.h")}...')
    functions_ids = []
    with open("openclIDs.h", "r") as ids_old:
        for line in ids_old:
            elem = line.strip(",\n")
            equal_sign = line.find(" ")
            if equal_sign != -1:
                elem = elem[:equal_sign]
            if "ID" in elem:
                functions_ids.append(elem)

    with open("openclIDs.h", "a") as ids:
        for _, func in sorted(functions.items()):
            id_ = func["id"]
            if id_ not in functions_ids:
                ids.write(id_ + ",\n")

    with open("openclIDs.h", "r") as ids, open("../common/include/openclIDs.h", "w") as ids_gen:
        ids_gen.write(ids.read())

    files = [
        "openclPrePostAuto.cpp",
        "openclHeaderAuto.h",
        "openclRecorderWrapperIfaceAuto.h",
        "openclRecorderWrapperAuto.h",
        "openclRecorderWrapperAuto.cpp",
        "openclIDswitch.h",
        "openclFunctionsAuto.h",
        "openclFunctionsAuto.cpp",
        "openclDriversAuto.inl",
        "openclDriversInit.cpp",
        "openclArgumentsAuto.h",
        "openclArgumentsAuto.cpp",
        "openclTracingAuto.h",
        "oclPlugin.def",
        "clconstants.lua",
        "helperCLAuto.inl",
        "helperCLAuto.cpp",
    ]

    for f in files:
        print(f"Generating {os.path.join(pathout, f)}...")
        ret_val = _mako_write(
            os.path.join(pathin, "templates", f"{f}.mako"),
            os.path.join(pathout, f),
            functions=functions,
            enums=enums,
            arguments=arguments,
        )
        if ret_val != 0:
            sys.exit(ret_val)


if __name__ == "__main__":
    main()
