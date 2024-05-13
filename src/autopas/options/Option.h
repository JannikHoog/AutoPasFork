/**
 * @file Option.h
 * @author F. Gratl
 * @date 10/14/19
 */

#pragma once

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "autopas/utils/StringUtils.h"

namespace autopas {
inline namespace options {
/**
 * Base class for autopas options.
 * @tparam actualOption Curiously recurring template pattern.
 */
template <typename actualOption>
class Option {
 public:
  /**
   * Prevents cast to bool by deleting the conversion operator.
   * @return
   */
  explicit operator bool() = delete;

  /**
   * Provides a way to iterate over the possible options.
   * @return Set of all possible values of this option type.
   */
  static std::set<actualOption> getAllOptions() {
    const auto mapOptionNames = actualOption::getOptionNames();
    std::set<actualOption> retSet;
    std::for_each(mapOptionNames.begin(), mapOptionNames.end(),
                  [&](auto pairOpStr) { retSet.insert(pairOpStr.first); });
    return retSet;
  };

  /**
   * Provides a way to iterate over the possible options minus those that are very unlikely to be on interest.
   * @note This function is meant to provide sane defaults.
   * @return
   */
  static std::set<actualOption> getMostOptions() {
    const auto allOptions = getAllOptions();
    const auto discouragedOptions = actualOption::getDiscouragedOptions();
    std::set<actualOption> retSet;
    std::set_difference(allOptions.begin(), allOptions.end(), discouragedOptions.begin(), discouragedOptions.end(),
                        std::inserter(retSet, retSet.begin()));
    return retSet;
  }

  /**
   * Converts an Option object to its respective string representation.
   * @param fixedLength Whether result should be filled up with spaces to the length of the longest option.
   * @return The string representation or "Unknown Option (<IntValue>)".
   */
  [[nodiscard]] std::string to_string(bool fixedLength = false) const {
    const auto &actualThis = *static_cast<const actualOption *>(this);
    const auto mapOptNames = actualOption::getOptionNames();  // <- not copying the map destroys the strings
    const auto match = mapOptNames.find(actualThis);
    if (match == mapOptNames.end()) {
      return "Unknown Option (" + std::to_string(actualThis) + ")";
    } else {
      std::string result = match->second;
      if (fixedLength) {
        result.resize(maxStringLength(), ' ');
      }
      return result;
    }
  }

  /**
   * Returns the number of characters in the string representation of the longest option. Useful for pretty output.
   * @return the number of characters in the string representation of the longest option.
   */
  [[nodiscard]] static size_t maxStringLength() {
    static size_t maxLength = 0;
    if (maxLength == 0) {
      for (const auto &[_, name] : actualOption::getOptionNames()) {
        maxLength = std::max(maxLength, name.size());
      }
    }
    return maxLength;
  }

  /**
   * Converts a string of options to a set of enums. For best results, the options are expected to be lower case.
   *
   * Allowed delimiters can be found in autopas::utils::StringUtils::delimiters.
   * Possible options can be found in getAllOptions().
   *
   * This function uses the Needleman-Wunsch algorithm to find the closest matching options.
   * If an option is ambiguous an exception is thrown.
   *
   * @note If the only token in the string is "all", all options will be returned.
   *
   * @tparam OutputContainer Type of the container in which the parsed values are stored.
   * By default this will be a std::set to avoid duplicated but if ordering is important a std::vector could be used.
   * @param optionsString String containing traversal options.
   * @return Container of option enums. If no valid option was found the empty set is returned.
   */
  template <class OutputContainer = std::set<actualOption>>
  static OutputContainer parseOptions(const std::string &optionsString) {
    const auto needles = autopas::utils::StringUtils::tokenize(optionsString, autopas::utils::StringUtils::delimiters);

    // Shorthand to get everything
    if (needles.size() == 1 and needles.front() == "all") {
      if constexpr (std::is_same_v<OutputContainer, std::set<actualOption>>) {
        return actualOption::getAllOptions();
      } else {
        const auto allOptionsSet = actualOption::getAllOptions();
        OutputContainer allOptionsOut;
        std::copy(allOptionsSet.begin(), allOptionsSet.end(),
                  std::insert_iterator(allOptionsOut, allOptionsOut.begin()));
        return allOptionsOut;
      }
    }

    // create a map of enum -> string with lowercase enums as a lookup and fill strings in the haystack
    std::map<std::string, actualOption> allOptionNamesLower;
    std::vector<std::string> haystack;
    for (auto &[optionEnum, optionString] : actualOption::getOptionNames()) {
      std::transform(optionString.begin(), optionString.end(), optionString.begin(), ::tolower);
      allOptionNamesLower.emplace(optionString, optionEnum);
      haystack.push_back(optionString);
    }

    // convert all needles to options.
    OutputContainer foundOptions;
    std::transform(needles.begin(), needles.end(), std::insert_iterator(foundOptions, foundOptions.begin()),
                   [&](const auto &needle) {
                     // first find the best matching string
                     const auto matchingString = autopas::utils::StringUtils::matchStrings(haystack, needle);
                     // then find the corresponding enum and add it to the return set
                     return allOptionNamesLower[matchingString];
                   });

    return foundOptions;
  }

  /**
   * Converts a string to an enum.
   *
   * This function works faster than parseOptions, however, the given string needs to match exactly an option.
   *
   * @tparam lowercase if set to true all option names are transformed to lower case.
   * @param optionString
   * @return Option enum.
   */
  template <bool lowercase = false>
  static actualOption parseOptionExact(const std::string &optionString) {
    for (auto [optionEnum, optionName] : actualOption::getOptionNames()) {
      if constexpr (lowercase) {
        std::transform(std::begin(optionName), std::end(optionName), std::begin(optionName), ::tolower);
      }
      if (optionString == optionName) {
        return optionEnum;
      }
    }

    // the end of the function should not be reached
    utils::ExceptionHandler::exception("Option::parseOptionExact() no match found for: {}", optionString);
    return actualOption();
  }

  /**
   * Stream output operator.
   * @param os
   * @param option
   * @return
   */
  friend std::ostream &operator<<(std::ostream &os, const Option &option) {
    os << option.to_string();
    return os;
  }

  /**
   * Stream extraction operator.
   * @param in Stream from which will be parsed.
   * @param option out-parameter that will be filled with the parsed option.
   * @return Processed stream.
   */
  friend std::istream &operator>>(std::istream &in, actualOption &option) {
    // Buffer for the current char.
    char c = ' ';
    // Skip any leading whitespace
    while (std::iswspace(c)) {
      in.get(c);
    }
    // String that is parsed.
    std::string str{c};
    // Concat c to str until an unexpected char is encountered
    do {
      in.get(c);
      str.push_back(c);
    } while (std::isalnum(c) or c == '_' or c == '-');  // This assumes that an option only contains alphanum, _, or -.
    // Pop the last char which is an undesired one.
    str.pop_back();

    // Actual parsing.
    option = parseOptionExact(str);
    return in;
  }
};
}  // namespace options
}  // namespace autopas
