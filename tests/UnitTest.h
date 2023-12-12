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

#include "core/Timer.h"


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
            failedLog << "Actual: " << arg1;
            failedLog << " vs Expected: " << arg2 << "\n\n";
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
