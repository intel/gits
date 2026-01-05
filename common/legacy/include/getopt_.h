// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   getopt.h
 *
 * @brief  Declaration of command line argument parser classes
 *
 */

#pragma once

#include "platform.h"

#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <filesystem>

#include "pragmas.h"

namespace {
template <typename Target, typename Source>
inline Target lexical_cast(const Source& arg) {
  std::stringstream sstream;
  Target result;

  sstream << arg;
  sstream >> result;
  if (sstream) {
    return result;
  }
  throw std::runtime_error("Bad lexical_cast");
}
} // namespace

namespace gits {

enum OptionGroup {
  OPTION_GROUP_GENERAL,
  OPTION_GROUP_PLAYBACK,
  OPTION_GROUP_IMAGE,
  OPTION_GROUP_METRICS,
  OPTION_GROUP_PERFORMANCE,
  OPTION_GROUP_MUTATORS,
  OPTION_GROUP_WORKAROUND,
  OPTION_GROUP_INTERNAL,
  OPTION_GROUP_INVALID,
};

class COption;

/**
   * @brief Command line arguments parser
   *
   * CGetOpt class is responsible for parsing options specified by
   * the user in command line while running the application. The
   * class contains a list of supported options. Options are determined
   * by its short or long identifier. Each option can have optional or
   * mandatory argument. CGetOpt class is also responsible for dumping
   * 'usage' screen of the application.
   */
class CGetOpt {
private:
  static const unsigned int LINE_WIDTH =
      99; /**< @brief maximum line size of the usage information */
  static const unsigned int DESC_INDENT =
      40; /**< @brief indentation of option description in usage screen */
  static const unsigned int ARGS_WIDTH =
      10; /**< @brief space needed to write option argument in usage screen */

  std::filesystem::path _appPath; /**< @brief application path */
  std::string _appName;           /**< @brief application name string */
  int& _argcRef;                  /**< @brief reference of application arguments number */
  const int _argc;                /**< @brief local copy of original arguments number */
  char**& _argvRef;               /**< @brief reference to application start of arguments table */
  char** const _argv;             /**< @brief local copy of original start of arguments table */
  std::vector<COption*>
      _optionsList; /**< @brief the list of options allowed for that application */
  std::vector<std::string> _notConsumed;

  void UsageOptions(const std::string& groupName,
                    unsigned lineWidth,
                    unsigned descIndent,
                    unsigned argsWidth) const;

public:
  CGetOpt(int& argc, char**& argv);
  ~CGetOpt() = default;
  CGetOpt(const CGetOpt& other) = delete;
  CGetOpt& operator=(const CGetOpt& other) = delete;

  const std::filesystem::path& AppPath() const;
  const std::string& AppName() const;

  void OptionAdd(COption& option);
  void Parse();

  void Usage(const std::string& groupName,
             unsigned lineWidth = LINE_WIDTH,
             unsigned descIndent = DESC_INDENT,
             unsigned argsWidth = ARGS_WIDTH) const;

  const std::vector<std::string>& NotConsumed() const {
    return _notConsumed;
  }
};

/**
    * @brief Command line option data
    *
    * CGetOpt::COption class is responsible for actions connected to
    * one option supported by an application. Option is determined
    * by its short or long identifier. It can have optional or
    * mandatory argument.
    */
class COption {
public:
  /**
      * @brief Option argument types
      */
  enum TArgumentType {
    ARGUMENT_NO,       /**< @brief no argument for that option */
    ARGUMENT_MANDATORY /**< @brief argument is mandatory */
  };

private:
  const char _shortName;             /**< @brief option letter code to be used in short form */
  const std::string _longName;       /**< @brief option string name to be used in long form */
  const TArgumentType _argumentType; /**< @brief specifies if argument is needed for that option */
  const std::string _description;    /**< @brief string that describe option in 'Usage' screen */

  bool _valid;           /**< @brief flag that specifies if option was found CLI */
  std::string _argument; /**< @brief if argument is present it will be stored here */
  OptionGroup _group;

  void Valid(bool flag);
  void Argument(const std::string& value);

  virtual bool ArgumentCheck();

  COption(const COption&);
  const COption& operator=(const COption&);

protected:
  friend class CGetOpt;

  COption(CGetOpt& parser,
          OptionGroup group,
          char shortName,
          std::string longName,
          TArgumentType argumentType,
          std::string description,
          unsigned supportedPlatforms);
  virtual ~COption();

  char ShortName() const;

  TArgumentType ArgumentType() const;
  OptionGroup Group() const;
  const std::string& Description() const;

  bool Valid() const;
  const std::string* Argument() const;

  virtual const char* ArgumentName() const;

public:
  const std::string& LongName() const;
};

template <class T>
class TypedOption : public COption {
public:
  TypedOption(CGetOpt& parser,
              OptionGroup group,
              char shortName,
              const std::string& longName,
              const std::string& description,
              unsigned supportedPlatforms = GITS_PLATFORM_BIT_ALL)
      : COption(parser,
                group,
                shortName,
                longName,
                ARGUMENT_MANDATORY,
                description,
                supportedPlatforms) {}
  bool Present() const {
    return Valid();
  }
  std::string StrValue() {
    const std::string* value = Argument();
    if (value) {
      return std::string(*value);
    }
    throw std::runtime_error("Value of unavailable argument requested");
  }
  T Value() const {
    const std::string* value = Argument();
    if (value) {
      return ::lexical_cast<T>(*value);
    }
    throw std::runtime_error("Value of unavailable argument requested");
  }
};

template <>
class TypedOption<std::filesystem::path> : public COption {
public:
  TypedOption(CGetOpt& parser,
              OptionGroup group,
              char shortName,
              const std::string& longName,
              const std::string& description,
              unsigned supportedPlatforms = GITS_PLATFORM_BIT_ALL)
      : COption(parser,
                group,
                shortName,
                longName,
                ARGUMENT_MANDATORY,
                description,
                supportedPlatforms) {}
  bool Present() const {
    return Valid();
  }
  std::filesystem::path Value() const {
    const std::string* value = Argument();
    if (value) {
      return std::filesystem::path(*value);
    }
    throw std::runtime_error("Value of unavailable argument requested");
  }
};

template <>
class TypedOption<bool> : public COption {
public:
  TypedOption(CGetOpt& parser,
              OptionGroup group,
              char shortName,
              const std::string& longName,
              const std::string& description,
              unsigned supportedPlatforms = GITS_PLATFORM_BIT_ALL)
      : COption(parser, group, shortName, longName, ARGUMENT_NO, description, supportedPlatforms) {}
  bool Present() const {
    return Valid();
  }
  bool Value() const {
    return Valid();
  }
};
} // namespace gits
