#include "testPch.h"
#include "onnxruntime_cxx_api.h"
#include "test/onnx/TestCase.h"
#include "test/onnx/heap_buffer.h"
#include "test/util/include/test/compare_ortvalue.h"
#include "ort_value_helper.h"
#include "StringHelpers.h"

#ifndef BUILD_GOOGLE_TEST
#error Must use googletest for value-parameterized tests
#endif

using namespace onnxruntime::test;
using namespace winml;
using namespace onnxruntime;

namespace WinML {
// Global needed to keep the actual ITestCase alive while the tests are going on. Only ITestCase* are used as test parameters.
std::vector<std::unique_ptr<ITestCase>> ownedTests;

class ModelTest : public testing::TestWithParam<std::tuple<ITestCase*, winml::LearningModelDeviceKind>> {
 protected:
  void SetUp() override {
    std::tie(m_testCase, m_deviceKind) = GetParam();
    WINML_EXPECT_NO_THROW(m_testCase->GetPerSampleTolerance(&m_perSampleTolerance));
    WINML_EXPECT_NO_THROW(m_testCase->GetRelativePerSampleTolerance(&m_relativePerSampleTolerance));
    WINML_EXPECT_NO_THROW(m_testCase->GetPostProcessing(&m_postProcessing));
  }
  // Called after the last test in this test suite.
  static void TearDownTestSuite() {
    ownedTests.clear(); // clear the global vector
  } 
  winml::LearningModelDeviceKind m_deviceKind;
  ITestCase* m_testCase;
  double m_perSampleTolerance = 1e-3;
  double m_relativePerSampleTolerance = 1e-3;
  bool m_postProcessing = false;

  void BindInputsFromFeed(LearningModelBinding& binding, std::unordered_map<std::string, Ort::Value>& feed) {
    for (auto& [name, value] : feed) {
      ITensor bindingValue;
      WINML_EXPECT_NO_THROW(bindingValue = OrtValueHelpers::LoadTensorFromOrtValue(value));
      WINML_EXPECT_NO_THROW(binding.Bind(_winml::Strings::WStringFromString(name), bindingValue));
    }
  }

  void CompareEvaluationResults(LearningModelEvaluationResult& results,
                                std::unordered_map<std::string, Ort::Value>& expectedOutputFeeds) {
    for (auto& [name, value] : expectedOutputFeeds) {
      // Extract the output buffer from the evaluation output
      std::wstring outputName = _winml::Strings::WStringFromString(name);
      auto actualOutputTensorValue = results.Outputs().Lookup(outputName).as<ITensorNative>();
      BYTE* actualData;
      uint32_t actualSizeInBytes;
      WINML_EXPECT_HRESULT_SUCCEEDED(actualOutputTensorValue->GetBuffer(&actualData, &actualSizeInBytes));

      // Create a copy of Ort::Value from evaluation output
      auto expectedShapeAndTensorType = Ort::TensorTypeAndShapeInfo{nullptr};
      auto memoryInfo = Ort::MemoryInfo{nullptr};
      WINML_EXPECT_NO_THROW(expectedShapeAndTensorType = value.GetTensorTypeAndShapeInfo());
      WINML_EXPECT_NO_THROW(memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
      Ort::Value actualOutput = Ort::Value{nullptr};
      WINML_EXPECT_NO_THROW(
          actualOutput = Ort::Value::CreateTensor(
              memoryInfo,
              actualData,
              actualSizeInBytes,
              expectedShapeAndTensorType.GetShape().data(),
              expectedShapeAndTensorType.GetShape().size(),
              expectedShapeAndTensorType.GetElementType()));

      // Use the expected and actual OrtValues to compare
      std::pair<COMPARE_RESULT, std::string> ret = CompareOrtValue(*actualOutput, *value, m_perSampleTolerance, m_relativePerSampleTolerance, m_postProcessing);
      WINML_EXPECT_EQUAL(COMPARE_RESULT::SUCCESS, ret.first) << ret.second;
    }
  }
};

TEST_P(ModelTest, Run) {
  LearningModel model = nullptr;
  LearningModelDevice device = nullptr;
  LearningModelSession session = nullptr;
  LearningModelBinding binding = nullptr;
  WINML_EXPECT_NO_THROW(model = LearningModel::LoadFromFilePath(m_testCase->GetModelUrl()));
  WINML_EXPECT_NO_THROW(device = LearningModelDevice(m_deviceKind));
  WINML_EXPECT_NO_THROW(session = LearningModelSession(model, device));
  WINML_EXPECT_NO_THROW(binding = LearningModelBinding(session));
  for (int i = 0; i < m_testCase->GetDataCount(); i++) {

    // Load and bind inputs
    onnxruntime::test::HeapBuffer inputHolder;
    std::unordered_map<std::string, Ort::Value> inputFeeds;
    WINML_EXPECT_NO_THROW(m_testCase->LoadTestData(i, inputHolder, inputFeeds, true));
    WINML_EXPECT_NO_THROW(BindInputsFromFeed(binding, inputFeeds));

    // evaluate
    LearningModelEvaluationResult results = nullptr;
    WINML_EXPECT_NO_THROW(results = session.Evaluate(binding, L"Testing"));

    // Load expected outputs
    onnxruntime::test::HeapBuffer outputHolder;
    std::unordered_map<std::string, Ort::Value> outputFeeds;
    WINML_EXPECT_NO_THROW(m_testCase->LoadTestData(i, outputHolder, outputFeeds, false));

    // compare results
    CompareEvaluationResults(results, outputFeeds);
  }
}

void SetTestDataPath(char* testDataPath) {
  GetEnvironmentVariableA("WINML_TEST_DATA_PATH", testDataPath, MAX_PATH);
  if (testDataPath[0] == 0) {
    throw std::invalid_argument("need to set environment variable WINML_TEST_DATA_PATH");
  }
}

// This function returns the list of all test cases inside model test collateral
static std::vector<ITestCase*> GetAllTestCases() {
  std::vector<ITestCase*> tests;
  std::vector<std::basic_string<PATH_CHAR_TYPE>> whitelistedTestCases;
  double perSampleTolerance = 1e-3;
  double relativePerSampleTolerance = 1e-3;
  std::unordered_set<std::basic_string<ORTCHAR_T>> allDisabledTests;
  std::vector<std::basic_string<PATH_CHAR_TYPE>> dataDirs;
  char testDataPath[MAX_PATH];
  SetTestDataPath(testDataPath);

  for (auto& p : std::filesystem::directory_iterator(testDataPath)) {
    if (p.is_directory()) {
      dataDirs.push_back(std::move(p.path()));
    }
  }

  WINML_EXPECT_NO_THROW(LoadTests(dataDirs, whitelistedTestCases, perSampleTolerance, relativePerSampleTolerance,
                                  allDisabledTests,
                                  [&tests](std::unique_ptr<ITestCase> l) {
                                    tests.push_back(l.get());
                                    ownedTests.push_back(std::move(l));
                                  }));
  return tests;
}

// This function gets the name of the test
static std::string GetNameOfTest(const testing::TestParamInfo<ModelTest::ParamType>& info) {
  std::string name = "";
  auto modelPath = std::wstring(std::get<0>(info.param)->GetModelUrl());
  auto modelPathStr = _winml::Strings::UTF8FromUnicode(modelPath.c_str(), modelPath.length());
  std::vector<std::string> tokenizedModelPath;
  std::istringstream ss(modelPathStr);
  std::string token;
  while (std::getline(ss, token, '\\')) {
    tokenizedModelPath.push_back(std::move(token));
  }
  // The model path is structured like this "<opset>/<model_name>/model.onnx
  // The desired naming of the test is like this <model_name>_<opset>_<CPU/GPU>
  name += tokenizedModelPath[tokenizedModelPath.size() - 2] += "_"; // model name
  name += tokenizedModelPath[tokenizedModelPath.size() - 3] += "_"; // opset version

  if (std::get<1>(info.param) == winml::LearningModelDeviceKind::Cpu) {
    name += "CPU";
  } else {
    name += "GPU";
  }

  std::replace_if(name.begin(), name.end(), [&name](char c) { return !google::protobuf::ascii_isalnum(c); }, '_');
  return name;
}

INSTANTIATE_TEST_SUITE_P(ModelTests, ModelTest, testing::Combine(testing::ValuesIn(GetAllTestCases()), testing::Values(winml::LearningModelDeviceKind::Cpu, winml::LearningModelDeviceKind::DirectX)),
                         GetNameOfTest);
}  // namespace WinML