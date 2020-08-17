#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <arrow/api.h>

#include "node.h"
#include "data_handlers/data_handler.h"

class EvalNode : public Node {
 public:
  template <typename U>
  EvalNode(const std::string& name,
           U&& consumers,
           std::shared_ptr<DataHandler> data_handler)
      : Node(name, std::forward<U>(consumers))
      , data_handler_(std::move(data_handler)) {
    configureNode();
  }

  void start() override;
  void handleData(const char *data, size_t length) override;
  void stop() override;

 private:
  void configureNode();

  arrow::Status processData(const std::shared_ptr<arrow::Buffer> &data_buffer,
                            std::shared_ptr<arrow::Buffer> &processed_data);

 private:
  std::shared_ptr<DataHandler> data_handler_;
};


