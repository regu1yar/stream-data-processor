#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <arrow/api.h>

#include "metadata.pb.h"
#include "udf.pb.h"

namespace stream_data_processor {
namespace kapacitor_udf {
namespace convert_utils {

class PointsConverter {
 public:
  struct PointsToRecordBatchesConversionOptions {
    std::string time_column_name;
    std::string measurement_column_name;
  };

 public:
  static arrow::Result<arrow::RecordBatchVector> convertToRecordBatches(
      const agent::PointBatch& points,
      const PointsToRecordBatchesConversionOptions& options);

  static arrow::Result<agent::PointBatch> convertToPoints(
      const arrow::RecordBatchVector& record_batches);

 private:
  static arrow::Result<std::shared_ptr<arrow::RecordBatch>>
  convertPointsGroup(const agent::PointBatch& points,
                     const std::string& group_string,
                     const std::vector<size_t>& group_indexes,
                     const PointsToRecordBatchesConversionOptions& options);

  template <typename T, typename BuilderType>
  static void addBuilders(
      const google::protobuf::Map<std::string, T>& data_map,
      std::map<std::string, BuilderType>* builders,
      arrow::MemoryPool* pool = arrow::default_memory_pool());

  template <typename T, typename BuilderType>
  static arrow::Status appendValues(
      const google::protobuf::Map<std::string, T>& data_map,
      std::map<std::string, BuilderType>* builders);

  template <typename BuilderType>
  static arrow::Status buildColumnArrays(
      arrow::ArrayVector* column_arrays, arrow::FieldVector* schema_fields,
      std::map<std::string, BuilderType>* builders,
      const std::shared_ptr<arrow::DataType>& data_type,
      metadata::ColumnType column_type);
};

}  // namespace convert_utils
}  // namespace kapacitor_udf
}  // namespace stream_data_processor
