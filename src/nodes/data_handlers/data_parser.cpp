#include <memory>
#include <utility>
#include <vector>

#include <arrow/csv/api.h>
#include <arrow/io/api.h>

#include "data_parser.h"
#include "utils/serialize_utils.h"

namespace stream_data_processor {

DataParser::DataParser(std::shared_ptr<Parser> parser)
    : parser_(std::move(parser)) {}

arrow::Result<arrow::BufferVector> DataParser::handle(
    const arrow::Buffer& source) {
  std::vector<std::shared_ptr<arrow::RecordBatch>> record_batches;
  ARROW_ASSIGN_OR_RAISE(record_batches, parser_->parseRecordBatches(source));
  if (record_batches.empty()) {
    return arrow::Status::OK();
  }

  return serialize_utils::serializeRecordBatches(record_batches);
}

}  // namespace stream_data_processor
