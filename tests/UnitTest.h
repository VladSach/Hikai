#ifndef HK_UNIT_TEST_H
#define HK_UNIT_TEST_H

#define DEFINE_TEST_WO_GROUP(name, test) \
    UnitTest::instance().addTest("anonimous", name, [&]() {test; return true;});
#define DEFINE_TEST_W_GROUP(group, name, test) \
    UnitTest::instance().addTest(group, name, [&]() {test; return true;});

#define EXPAND( x ) x
#define GET_MACRO(group, name, test, FUNC, ...) FUNC

#define DEFINE_TEST(...)                    \
    EXPAND(GET_MACRO(__VA_ARGS__,           \
                     DEFINE_TEST_W_GROUP,   \
                     DEFINE_TEST_WO_GROUP,) \
                     (__VA_ARGS__))

#define RUN_ALL_TESTS() UnitTest::instance().runAllTests()
#define RUN_GROUP_TESTS(group) UnitTest::instance().runTests(group)

#define EXPECT_EQ(arg1, arg2)                         \
    if (!UnitTest::instance().expectEQ(arg1, arg2)) { \
        return false;                                 \
    }


#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "core/Timer.h"
#include "math/hkmath.h"

inline std::ostream& operator<<(std::ostream& out, const hkm::mat4f& m)
{
    return out << '\n'
        << m(0, 0) << " " << m(0, 1) << " " << m(0, 2) << " " << m(0, 3) << '\n'
        << m(1, 0) << " " << m(1, 1) << " " << m(1, 2) << " " << m(1, 3) << '\n'
        << m(2, 2) << " " << m(2, 1) << " " << m(2, 2) << " " << m(2, 3) << '\n'
        << m(3, 3) << " " << m(3, 1) << " " << m(3, 2) << " " << m(3, 3) <<'\n';
}

inline std::ostream& operator<<(std::ostream& out, const hkm::mat3f& m)
{
    constexpr u32 width = 7;
    return out << '\n'
        << std::setw(width) << m(0, 0) << " "
        << std::setw(width) << m(0, 1) << " "
        << std::setw(width) << m(0, 2) << " "
        << '\n'

        << std::setw(width) << m(1, 0) << " "
        << std::setw(width) << m(1, 1) << " "
        << std::setw(width) << m(1, 2) << " "
        << '\n'

        << std::setw(width) << m(2, 0) << " "
        << std::setw(width) << m(2, 1) << " "
        << std::setw(width) << m(2, 2) << " "
        << '\n';
}

inline std::ostream& operator<<(std::ostream& out, const hkm::quaternion& quat)
{
    return out
        << quat.x << " " << quat.y << " " << quat.z << " " << quat.w << '\n';
}

class UnitTest {
protected:
    UnitTest() {}

public:
    virtual ~UnitTest() {}
    virtual bool runTest() { return false; }

    UnitTest(UnitTest &other) = delete;
    void operator=(const UnitTest&) = delete;

    static UnitTest& instance() {
        static UnitTest singleton;
        return singleton;
    }

    void addTest(const std::string &group,
                 const std::string &name,
                 const std::function<bool()> &test);

    void runTests(const std::string &group);
    void runAllTests();


    template<typename T>
    constexpr bool expectEQ(const T& arg1, const T& arg2) {
        bool result = (arg1 == arg2);

        if (!result) {
            failedLog << "- failed: " << getCurrentTestName() << '\n';
            failedLog << "Actual:   " << arg1;
            failedLog << "Expected: " << arg2 << "\n\n";
        }

        return result;
    }

    std::string getCurrentTestName() { return currentTestName; }

private:

    struct TestGroup {
        std::string name;
        std::function<bool()> test;
    };

    std::unordered_map<
        std::string,           // group name
        std::vector<TestGroup> // test cases
    > tests;

    std::ostringstream log;

    std::string currentTestName;
    std::ostringstream failedLog;

    int totalPassed = 0;
    int totalFailed = 0;
    double totalTime = 0.f;
};

#endif // HK_UNIT_TEST_H
