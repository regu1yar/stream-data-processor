#pragma once

#include <memory>

#include "record_batch_handlers/record_batch_handler.h"

namespace stream_data_processor {

class StatefulHandler : public RecordBatchHandler {
 public:
  virtual std::shared_ptr<StatefulHandler> clone() const = 0;
};

}  // namespace stream_data_processor
