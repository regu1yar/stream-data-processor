#include <spdlog/spdlog.h>

#include "node.h"

namespace stream_data_processor {

void Node::passData(const std::vector<std::shared_ptr<arrow::Buffer>>& data) {
  for (auto& buffer : data) {
    spdlog::get(name_)->info("Passing data of size {}", buffer->size());
    for (auto& consumer : consumers_) {
      try {
        consumer->consume(buffer);
      } catch (const std::exception& e) {
        spdlog::get(name_)->error(e.what());
      }
    }
  }
}

void Node::log(const std::string& message, spdlog::level::level_enum level) {
  spdlog::get(name_)->log(level, message);
}

void Node::addConsumer(const std::shared_ptr<Consumer>& consumer) {
  consumers_.push_back(consumer);
}

const std::string& Node::getName() const { return name_; }

}  // namespace stream_data_processor
