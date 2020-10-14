#pragma once

#include <exception>
#include <regex>
#include <string>
#include <unordered_map>

#include <google/protobuf/map.h>
#include <google/protobuf/repeated_field.h>

#include "record_batch_handlers/aggregate_handler.h"

#include "udf.pb.h"

class InvalidOptionException : public std::exception {
 public:
  explicit InvalidOptionException(const std::string& message)
      : message_(message) {}

  [[nodiscard]] const char* what() const noexcept override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

class AggregateOptionsParser {
 public:
  static google::protobuf::Map<std::string, agent::OptionInfo>
  getResponseOptionsMap();
  static AggregateHandler::AggregateOptions parseOptions(
      const google::protobuf::RepeatedPtrField<agent::Option>&
          request_options);

 public:
  static const std::string AGGREGATES_OPTION_NAME;
  static const std::string TIME_AGGREGATE_RULE_OPTION_NAME;

 private:
  static void parseAggregates(
      const agent::Option& aggregates_request_option,
      AggregateHandler::AggregateOptions* aggregate_options);

  static void parseTimeAggregateRule(
      const agent::Option& time_aggregate_rule_option,
      AggregateHandler::AggregateOptions* aggregate_options);

 private:
  static const std::unordered_map<std::string,
                                  AggregateHandler::AggregateFunctionEnumType>
      FUNCTION_NAMES_TO_TYPES;
  static const std::regex AGGREGATE_STRING_REGEX;
};