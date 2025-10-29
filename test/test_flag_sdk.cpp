#include "../src/flag_sdk.h"
#include <iostream>
#include <cassert>

void testGetBooleanValue() {
    std::cout << "Running testGetBooleanValue..." << std::endl;

    // Test with various flag keys
    bool result1 = flagsdk::getBooleanValue("test-flag-1");
    assert(result1 == true);
    std::cout << "  ✓ getBooleanValue('test-flag-1') returned true" << std::endl;

    bool result2 = flagsdk::getBooleanValue("test-flag-2");
    assert(result2 == true);
    std::cout << "  ✓ getBooleanValue('test-flag-2') returned true" << std::endl;

    bool result3 = flagsdk::getBooleanValue("another-flag");
    assert(result3 == true);
    std::cout << "  ✓ getBooleanValue('another-flag') returned true" << std::endl;

    std::cout << "testGetBooleanValue passed!" << std::endl << std::endl;
}

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "Running Flag SDK Tests" << std::endl;
    std::cout << "==================================" << std::endl << std::endl;

    try {
        testGetBooleanValue();

        std::cout << "==================================" << std::endl;
        std::cout << "All tests passed!" << std::endl;
        std::cout << "==================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
