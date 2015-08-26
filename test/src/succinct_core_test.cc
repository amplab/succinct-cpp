#include "succinct_core.h"

#include "gtest/gtest.h"

#include <iostream>
#include <unistd.h>
#define GetCurrentDir getcwd

std::string data_path;

class SuccinctCoreTest : public testing::Test {
 protected:
  virtual void SetUp() {
    s_core = new SuccinctCore((data_path + "/test_file").c_str());
  }

  virtual void TearDown() {
    delete s_core;
  }

  std::string LoadDataFromFile(std::string filename) {
    std::ifstream input(filename);
    std::string data;
    return data;
  }

  std::vector<uint64_t> LoadArrayFromFile(std::string filename) {
    std::ifstream input(filename);
    std::vector<uint64_t> SA;
    uint64_t sa_size;
    input.read(reinterpret_cast<char *>(&sa_size), sizeof(uint64_t));
    for (uint64_t i = 0; i < sa_size; i++) {
      uint64_t sa_val;
      input.read(reinterpret_cast<char *>(&sa_val), sizeof(uint64_t));
      SA.push_back(sa_val);
    }
    input.close();
    return SA;
  }

  SuccinctCore *s_core;

};

TEST_F(SuccinctCoreTest, LookupNPATest) {
  std::vector<uint64_t> NPA = LoadArrayFromFile(data_path + "/test_file.npa");

  for (uint64_t i = 0; i < NPA.size(); i++) {
    ASSERT_EQ(NPA[i], s_core->LookupNPA(i));
  }
}

TEST_F(SuccinctCoreTest, LookupSATest) {
  std::vector<uint64_t> SA = LoadArrayFromFile(data_path + "/test_file.sa");

  for (uint64_t i = 0; i < SA.size(); i++) {
    ASSERT_EQ(SA[i], s_core->LookupSA(i));
  }
}

TEST_F(SuccinctCoreTest, LookupISATest) {
  std::vector<uint64_t> ISA = LoadArrayFromFile(data_path + "/test_file.isa");

  for (uint64_t i = 0; i < ISA.size(); i++) {
    ASSERT_EQ(ISA[i], s_core->LookupISA(i));
  }
}
