#include "gtest/gtest.h"

extern std::string data_path;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    assert(argc == 2);
    data_path = (const std::string)std::string(argv[1]);
    return RUN_ALL_TESTS();
}
