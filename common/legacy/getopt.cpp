// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   getopt.cpp
 *
 * @brief  Implementation of command line argument parser classes
 *
 */

#include "getopt_.h"
#include "tools_lite.h"

#include <iomanip>
#include <algorithm>
#include <cstring>
#include <iostream>

namespace gits {

/**
 * @brief CGetOpt class constructor
 *
 * @param argc reference to a number of arguments given to the application
 * @param argv reference to arguments given to the application
 *
 * @note @p argc and @p argv application variables will be updated by
 *       the Parse() method.
 */
CGetOpt::CGetOpt(int& argc, char**& argv)
    : _argcRef(argc), _argc(argc), _argvRef(argv), _argv(argv) {
  // obtain application path and name
  std::filesystem::path argPath = std::filesystem::absolute(_argv[0]);
  _appPath = argPath.parent_path();
  _appName = argPath.filename().string();
}

const std::filesystem::path& CGetOpt::AppPath() const {
  return _appPath;
}

const std::string& CGetOpt::AppName() const {
  return _appName;
}

/**
 * @brief Adds new option to command line argument parser
 *
 * Method adds new option to command line argument parser.
 *
 * @note Ownership is not passed to CGetOpt class.
 *
 * @param option option to add
 */
void CGetOpt::OptionAdd(COption& option) {
  _optionsList.push_back(&option);
}

/**
 * @brief Parser application command line
 *
 * Method parses application command line. It checks all the arguments
 * from command line until it finds first argument that is not an option
 * (does not start from '-' sign). If an option is found than it is
 * marked as valid. If an option contain an argument it is stored in
 * an option and it is checked for correctness.
 *
 * @note @p argc and @p argv application variables are set to point to
 *       the first argument that is not an option.
 */
void CGetOpt::Parse() {
  int i = 1;

  // iterate through given CLI arguments
  // index 0 is application name so skip it
  for (; i < _argc; i++) {
    const char* ptr = _argv[i];
    size_t optLen = strlen(ptr);

    // check if it is an option
    if (ptr[0] == '-') {
      // it is an option
      if (optLen == 1) {
        throw std::runtime_error("GITS Option name not given!!!");
      }

      // iterate through all known options and compare
      auto it = _optionsList.begin();
      for (; it != _optionsList.end(); ++it) {
        if (ptr[1] == '-' || optLen > 2) {
          // it is a long form of an option name, also allow single dash in long options
          if (caseInsensitiveEquals((*it)->LongName(), &ptr[2]) ||
              caseInsensitiveEquals((*it)->LongName(), &ptr[1])) {
            // option found
            break;
          }
        } else {
          // it is a short form of an option name
          if (optLen != 2) {
            throw std::runtime_error(
                "GITS More than one character given for a short form of an option: " +
                std::string(ptr) + "!!!");
          }

          if (tolower((*it)->ShortName()) == tolower(ptr[1])) {
            // option found
            break;
          }
        }
      }

      if (it == _optionsList.end()) {
        // option not found
        throw std::runtime_error("GITS Option: " + std::string(ptr) + " not supported!!!");
      }

      // option found
      if ((*it)->Valid()) {
        // option given more than once
        throw std::runtime_error("GITS Option: " + std::string(ptr) +
                                 " can be specified only once!!!");
      }

      COption::TArgumentType argType = (*it)->ArgumentType();
      if (argType != COption::ARGUMENT_NO) {
        // check if argument is present
        bool found = false;

        if (i + 1 != _argc) {
          const char* nextPtr = _argv[i + 1];

          if (strlen(nextPtr) && nextPtr[0] != '-') {
            // argument present
            found = true;
          }
        }

        if (found) {
          // set argument in an option
          (*it)->Argument(_argv[i + 1]);

          // validate the argument
          if (!(*it)->ArgumentCheck()) {
            throw std::runtime_error("GITS Bad argument given: " + std::string(_argv[i + 1]) +
                                     " for an option: " + std::string(ptr) + "!!!");
          }

          // increment CLI argument index
          i++;
        } else {
          if (argType == COption::ARGUMENT_MANDATORY) {
            throw std::runtime_error(
                "GITS Mandatory argument not specified for option: " + std::string(ptr) + "!!!");
          }
        }
      }

      // set option as valid
      (*it)->Valid(true);
    } else {
      // Doesn't look like na option. Skip it.
      _notConsumed.push_back(ptr);
    }
  }
}

namespace {

struct OptCharPair {
  OptionGroup value;
  const char* name;
};
const OptCharPair optionGroupNameMapping[] = {
    {OPTION_GROUP_GENERAL, "general"},       {OPTION_GROUP_METRICS, "metrics"},
    {OPTION_GROUP_MUTATORS, "mutators"},     {OPTION_GROUP_PERFORMANCE, "performance"},
    {OPTION_GROUP_PLAYBACK, "playback"},     {OPTION_GROUP_IMAGE, "image"},
    {OPTION_GROUP_WORKAROUND, "workaround"}, {OPTION_GROUP_INTERNAL, "internal"},
};

} // namespace

OptionGroup stringToOptionGroup(const std::string& name) {
  for (const auto& elem : optionGroupNameMapping) {
    if (name == elem.name) {
      return elem.value;
    }
  }
  throw std::runtime_error("invalid option group name");
}

std::string optionGroupToString(OptionGroup value) {
  for (const auto& elem : optionGroupNameMapping) {
    if (value == elem.value) {
      return elem.name;
    }
  }
  throw std::runtime_error("invalid option group value");
}

/**
 * @brief Prints options description
 *
 * Method prints descriptions for all supported options.
 *
 * @param lineWidth maximum line size of the usage information
 * @param descIndent indentation of option description in usage screen
 * @param argsWidth space needed to write option argument in usage screen
 */
void CGetOpt::UsageOptions(const std::string& groupName,
                           unsigned lineWidth,
                           unsigned descIndent,
                           unsigned argsWidth) const {
  using namespace std;
  std::ios_base::fmtflags origFlags(cout.flags());

  auto opts = _optionsList;
  std::sort(opts.begin(), opts.end(),
            [](COption* lhs, COption* rhs) { return lhs->LongName() < rhs->LongName(); });
  std::stable_sort(opts.begin(), opts.end(),
                   [](COption* lhs, COption* rhs) { return lhs->Group() < rhs->Group(); });

  // Default group filter 'invalid' will pass everything.
  OptionGroup groupFilter = OPTION_GROUP_INVALID;
  if (groupName != "all") {
    groupFilter = stringToOptionGroup(groupName);
  }

  OptionGroup currentGroup = OPTION_GROUP_INVALID;
  for (auto& opt : opts) {
    if (groupFilter != OPTION_GROUP_INVALID && opt->Group() != groupFilter) {
      continue;
    }

    if (currentGroup != opt->Group()) {
      currentGroup = opt->Group();
      cout << "Option group: " << optionGroupToString(currentGroup) << "\n";
    }

    cout << "  ";

    // print short name of an option
    if (char shortName = opt->ShortName()) {
      cout << "-" << shortName << ", ";
    } else {
      cout << "    ";
    }

    // print long name of an option
    const string& longName = opt->LongName();
    size_t width = descIndent - argsWidth - 7;
    if (longName != "") {
      cout << "--" << setfill(' ') << setw(width - 2) << left << longName;
    } else {
      cout << setfill(' ') << setw(width) << " ";
    }
    cout << " ";

    // print option argument
    string argStr;
    switch (opt->ArgumentType()) {
    case COption::ARGUMENT_NO:
      break;

    case COption::ARGUMENT_MANDATORY:
      argStr = opt->ArgumentName();
      break;

    default:
      cerr << "Unknown argument type: " << opt->ArgumentType() << endl;
      throw std::runtime_error("Unknown argument type.");
    }
    cout << setfill(' ') << setw(argsWidth) << argStr;

    // print description text
    string description = opt->Description();
    const size_t descLine = lineWidth - descIndent - argsWidth - 10;
    while (!description.empty()) {
      if (description.size() != opt->Description().size()) {
        // not the first line
        cout << std::string(descIndent, ' ');
      }

      // Trim first space (this handles line breaks just before space).
      if (description[0] == ' ') {
        description = description.substr(1);
      }

      // Encountered newline, break on it.
      string::size_type lineBreak = description.find_first_of("\n");
      if (lineBreak != string::npos && lineBreak < descLine) {
        cout << description.substr(0, lineBreak + 1);
        description = description.substr(lineBreak + 1);
        continue;
      }

      // Description is shorter then limit, just output whole.
      if (description.size() < descLine) {
        cout << description << "\n";
        description.clear();
        continue;
      }

      // Description needs to be partitioned.
      lineBreak = description.find_last_of(" ", descLine);
      if (lineBreak != string::npos) {
        // Break on whitespace.
        cout << description.substr(0, lineBreak) << "\n";
        description = description.substr(lineBreak);
        continue;
      } else {
        // Need to break mid-word
        cout << description.substr(0, descLine - 1) << "-\n";
        description = description.substr(descLine - 1);
        continue;
      }
    }
    cout << "\n";
  }
  cout.flags(origFlags);
}

/**
 * @brief Prints usage screen
 *
 * Method prints usage screen.
 *
 * @param additionalParams parameters that are allowed by the
 *                         application and are not an option
 * @param lineWidth maximum line size of the usage information
 * @param descIndent indentation of option description in usage screen
 * @param argsWidth space needed to write option argument in usage screen
 */
void CGetOpt::Usage(const std::string& groupName,
                    unsigned lineWidth /* = LINE_WIDTH */,
                    unsigned descIndent /* = DESC_INDENT */,
                    unsigned argsWidth /* = ARGS_WIDTH*/) const {
  std::cout << "Options:" << std::endl;
  UsageOptions(groupName, lineWidth, descIndent, argsWidth);
}

/* ************************************ O P T I O N ************************************ */

/**
 * @brief CGetOpt::COption class constructor
 *
 * @param shortName option letter code to be used in short form
 * @param longName option string name to be used in long form
 * @param argumentType specifies if argument is needed for that option
 * @param description string that describe option in 'Usage' screen
 */
COption::COption(CGetOpt& parser,
                 OptionGroup group,
                 char shortName,
                 std::string longName,
                 TArgumentType argumentType,
                 std::string description,
                 unsigned supportedPlatforms)
    : _shortName(shortName),
      _longName(std::move(longName)),
      _argumentType(argumentType),
      _description(std::move(description)),
      _valid(false),
      _group(group) {
  if (supportedPlatforms & GITS_PLATFORM_BIT_CURRENT) {
    parser.OptionAdd(*this);
  }
}

/**
 * @brief CGetOpt::COption class destructor
 */
COption::~COption() {}

/**
 * @brief Marks an option as found in command line
 *
 * Method marks an option as found in application command line.
 *
 * @param flag @b true means that option was found
 */
void COption::Valid(bool flag) {
  _valid = flag;
}

/**
 * @brief Sets argument assigned to an option
 *
 * Method sets argument assigned to an option.
 *
 * @param value argument string value
 */
void COption::Argument(const std::string& value) {
  _argument = value;
}

/**
 * @brief Verifies argument correctness
 *
 * Method verifies option argument correctness. Can be used by
 * child classes to validate and convert given argument.
 *
 * @return @b true if option argument is correct
 */
bool COption::ArgumentCheck() {
  return true;
}

/**
 * @brief Returns short name identifier of the option
 *
 * Method returns short name identifier of the option.
 *
 * @return Short name identifier of the option
 */
char COption::ShortName() const {
  return _shortName;
}

/**
 * @brief Returns long name identifier of the option
 *
 * Method returns long name identifier of the option.
 *
 * @return Long name identifier of the option
 */
const std::string& COption::LongName() const {
  return _longName;
}

/**
 * @brief Returns information if option argument is expected
 *
 * Method returns information if option argument is expected.
 *
 * @return Information if option argument is expected
 */
COption::TArgumentType COption::ArgumentType() const {
  return _argumentType;
}

/**
 * @brief Returns option description string
 *
 * Method returns option description string.
 *
 * @return Option description string
 */
const std::string& COption::Description() const {
  return _description;
}

/**
 * @brief Returns true if option was found in application command line
 *
 * Method returns true if option was found in application command line.
 *
 * @return true if option was found in application command line
 */
bool COption::Valid() const {
  return _valid;
}

/**
 * @brief Returns option argument
 *
 * Method returns pointer to option argument or 0 if argument was not specified.
 *
 * @return Pointer to option argument or 0 if argument was not specified
 */
const std::string* COption::Argument() const {
  if (_argument == "") {
    return nullptr;
  }

  return &_argument;
}

/**
 * @brief Returns argument name that will be presented in 'Usage' screen
 *
 * Method returns argument name that will be presented in 'Usage' screen.
 *
 * @return Argument name that will be presented in 'Usage' screen
 */
const char* COption::ArgumentName() const {
  return "<ARG>";
}

OptionGroup COption::Group() const {
  return _group;
}

} // namespace gits
