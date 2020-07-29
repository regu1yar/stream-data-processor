#include <ctime>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <arrow/api.h>

#include <gtest/gtest.h>

#include "help.h"
#include "utils/parsers/graphite_parser.h"

TEST(GraphiteParserTest, SimpleTest) {
  std::vector<std::string> templates{"measurement.measurement.field.field.region"};
  std::shared_ptr<Parser> parser = std::make_shared<GraphiteParser>(templates, "_");
  auto metric_buffer = std::make_shared<arrow::Buffer>("cpu.usage.idle.percent.eu-east 100");
  arrow::RecordBatchVector record_batch_vector;
  ASSERT_TRUE(arrowAssertNotOk(parser->parseRecordBatches(metric_buffer, record_batch_vector)));

  ASSERT_EQ(1, record_batch_vector.size());
  checkSize(record_batch_vector[0], 1, 3);

  checkColumnsArePresent(record_batch_vector[0], {
    "measurement",
    "region",
    "idle_percent"
  });

  checkValue<std::string, arrow::StringScalar>("cpu_usage", record_batch_vector[0],
                                               "measurement", 0);
  checkValue<std::string, arrow::StringScalar>("eu-east", record_batch_vector[0],
                                               "region", 0);
  checkValue<int64_t, arrow::Int64Scalar>(100, record_batch_vector[0],
                                          "idle_percent", 0);
}

TEST(GraphiteParserTest, MultipleEndingTest) {
  std::vector<std::string> templates{"region.measurement*"};
  std::shared_ptr<Parser> parser = std::make_shared<GraphiteParser>(templates);
  auto metric_buffer = std::make_shared<arrow::Buffer>("us.cpu.load 100");
  arrow::RecordBatchVector record_batch_vector;
  ASSERT_TRUE(arrowAssertNotOk(parser->parseRecordBatches(metric_buffer, record_batch_vector)));

  ASSERT_EQ(1, record_batch_vector.size());
  checkSize(record_batch_vector[0], 1, 3);

  checkColumnsArePresent(record_batch_vector[0], {
      "measurement",
      "region",
      "value"
  });

  checkValue<std::string, arrow::StringScalar>("cpu.load", record_batch_vector[0],
                                               "measurement", 0);
  checkValue<std::string, arrow::StringScalar>("us", record_batch_vector[0],
                                               "region", 0);
  checkValue<int64_t, arrow::Int64Scalar>(100, record_batch_vector[0],
                                          "value", 0);
}

TEST(GraphiteParserTest, MultipleTemplatesTest) {
  std::vector<std::string> templates{
      "*.*.* region.region.measurement",
      "*.*.*.* region.region.host.measurement",
  };
  std::shared_ptr<Parser> parser = std::make_shared<GraphiteParser>(templates);
  auto metric_buffer = std::make_shared<arrow::Buffer>("cn.south.mem-usage 50\n"
                                                       "us.west.localhost.cpu 100");
  arrow::RecordBatchVector record_batch_vector;
  ASSERT_TRUE(arrowAssertNotOk(parser->parseRecordBatches(metric_buffer, record_batch_vector)));

  ASSERT_EQ(1, record_batch_vector.size());
  checkSize(record_batch_vector[0], 2, 4);

  checkColumnsArePresent(record_batch_vector[0], {
      "measurement",
      "region",
      "host",
      "value"
  });
  
  size_t metric_0 = 0;
  size_t metric_1 = 1;
  if (std::static_pointer_cast<arrow::StringScalar>(
      record_batch_vector[0]->GetColumnByName("measurement")->GetScalar(metric_0).ValueOrDie()
      )->value->ToString() != "mem") {
    std::swap(metric_0, metric_1);
  }

  checkValue<std::string, arrow::StringScalar>("mem-usage", record_batch_vector[0],
                                               "measurement", metric_0);
  checkValue<std::string, arrow::StringScalar>("cn.south", record_batch_vector[0],
                                               "region", metric_0);
  checkIsInvalid(record_batch_vector[0], "host", metric_0);
  checkValue<int64_t, arrow::Int64Scalar>(50, record_batch_vector[0],
                                          "value", metric_0);

  checkValue<std::string, arrow::StringScalar>("cpu", record_batch_vector[0],
                                               "measurement", metric_1);
  checkValue<std::string, arrow::StringScalar>("us.west", record_batch_vector[0],
                                               "region", metric_1);
  checkValue<std::string, arrow::StringScalar>("localhost", record_batch_vector[0],
                                               "host", metric_1);
  checkValue<int64_t, arrow::Int64Scalar>(100, record_batch_vector[0],
                                          "value", metric_1);
}

TEST(GraphiteParserTest, FilterTest) {
  std::vector<std::string> templates{
      "cpu.* measurement.measurement.region",
      "mem.* measurement.measurement.host"
  };
  std::shared_ptr<Parser> parser = std::make_shared<GraphiteParser>(templates);
  auto metric_buffer = std::make_shared<arrow::Buffer>("cpu.load.eu-east 100\n"
                                                       "mem.cached.localhost 256");
  arrow::RecordBatchVector record_batch_vector;
  ASSERT_TRUE(arrowAssertNotOk(parser->parseRecordBatches(metric_buffer, record_batch_vector)));

  ASSERT_EQ(1, record_batch_vector.size());
  checkSize(record_batch_vector[0], 2, 4);

  checkColumnsArePresent(record_batch_vector[0], {
      "measurement",
      "region",
      "host",
      "value"
  });

  size_t metric_0 = 0;
  size_t metric_1 = 1;
  if (std::static_pointer_cast<arrow::StringScalar>(
      record_batch_vector[0]->GetColumnByName("measurement")->GetScalar(metric_0).ValueOrDie()
  )->value->ToString() != "cpu.load") {
    std::swap(metric_0, metric_1);
  }

  checkValue<std::string, arrow::StringScalar>("cpu.load", record_batch_vector[0],
                                               "measurement", metric_0);
  checkValue<std::string, arrow::StringScalar>("eu-east", record_batch_vector[0],
                                               "region", metric_0);
  checkIsInvalid(record_batch_vector[0], "host", metric_0);
  checkValue<int64_t, arrow::Int64Scalar>(100, record_batch_vector[0],
                                          "value", metric_0);

  checkValue<std::string, arrow::StringScalar>("mem.cached", record_batch_vector[0],
                                               "measurement", metric_1);
  checkIsInvalid(record_batch_vector[0], "region", metric_1);
  checkValue<std::string, arrow::StringScalar>("localhost", record_batch_vector[0],
                                               "host", metric_1);
  checkValue<int64_t, arrow::Int64Scalar>(256, record_batch_vector[0],
                                          "value", metric_1);
}

TEST(GraphiteParserTest, AdditionalTagsTest) {
  std::vector<std::string> templates{
      "measurement.measurement.field.region datacenter=1a"
  };
  std::shared_ptr<Parser> parser = std::make_shared<GraphiteParser>(templates);
  auto metric_buffer = std::make_shared<arrow::Buffer>("cpu.usage.idle.eu-east 100");
  arrow::RecordBatchVector record_batch_vector;
  ASSERT_TRUE(arrowAssertNotOk(parser->parseRecordBatches(metric_buffer, record_batch_vector)));

  ASSERT_EQ(1, record_batch_vector.size());
  checkSize(record_batch_vector[0], 1, 4);

  checkColumnsArePresent(record_batch_vector[0], {
      "measurement",
      "region",
      "idle",
      "datacenter"
  });

  checkValue<std::string, arrow::StringScalar>("cpu.usage", record_batch_vector[0],
                                               "measurement", 0);
  checkValue<std::string, arrow::StringScalar>("eu-east", record_batch_vector[0],
                                               "region", 0);
  checkValue<std::string, arrow::StringScalar>("1a", record_batch_vector[0],
                                               "datacenter", 0);
  checkValue<int64_t, arrow::Int64Scalar>(100, record_batch_vector[0],
                                          "idle", 0);
}

TEST(GraphiteParserTest, TimestampTest) {
  std::vector<std::string> templates{"measurement.measurement.field.field.region"};
  std::shared_ptr<Parser> parser = std::make_shared<GraphiteParser>(templates);
  auto now = std::time(nullptr);
  std::stringstream metric_string_builder;
  metric_string_builder << "cpu.usage.idle.percent.eu-east 100 " << now;
  auto metric_buffer = std::make_shared<arrow::Buffer>(metric_string_builder.str());
  arrow::RecordBatchVector record_batch_vector;
  ASSERT_TRUE(arrowAssertNotOk(parser->parseRecordBatches(metric_buffer, record_batch_vector)));

  ASSERT_EQ(1, record_batch_vector.size());
  checkSize(record_batch_vector[0], 1, 4);

  checkColumnsArePresent(record_batch_vector[0], {
      "measurement",
      "region",
      "idle.percent",
      "timestamp"
  });

  checkValue<std::string, arrow::StringScalar>("cpu.usage", record_batch_vector[0],
                                               "measurement", 0);
  checkValue<std::string, arrow::StringScalar>("eu-east", record_batch_vector[0],
                                               "region", 0);
  checkValue<int64_t, arrow::Int64Scalar>(100, record_batch_vector[0],
                                          "idle.percent", 0);
  checkValue<int64_t, arrow::Int64Scalar>(now, record_batch_vector[0],
                                          "timestamp", 0);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}