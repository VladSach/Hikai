#include "UnitTest.h"

void UnitTest::addTest(const std::string &group,
                       const std::string &name,
                       const std::function<bool()> &test)
{
    tests[group].push_back({name, test});
}

void UnitTest::runTests(const std::string &group)
{
    int passed = 0;
    int failed = 0;
    hk::Timer timer;

    auto &groupTests = tests.at(group);

    if (groupTests.empty()) {
        log << "No tests registered for group: " << group << '\n';
    }

    timer.record();
    for (const auto &test : groupTests) {
        currentTestName = test.name;
        test.test() ? ++passed : ++failed;
    }
    double time = timer.update();

    totalPassed += passed;
    totalFailed += failed;
    totalTime += time;

    auto center = [](const auto& arg, int width) {
        std::ostringstream tmp;

        tmp << arg;
        std::string str = tmp.str();

        int len = static_cast<int>(str.length());
        if (width < len) { return str; }

        int diff = width - len;
        int pad1 = diff/2;
        int pad2 = diff - pad1;
        return std::string(pad1, ' ') + str + std::string(pad2, ' ');
    };

    log << "|";
    log << std::setw(14) << center(group, 14) << "|";
    log << std::setw(8)  << center(passed, 8) << "|";
    log << std::setw(8)  << center(failed, 8) << "|";

    log.setf(std::ios_base::fixed, std::ios_base::floatfield);
    log << std::setprecision(7) << ' ' << time << "s |";

    log << '\n';

    log << "-----------------------------------------------" << '\n';
}

void UnitTest::runAllTests()
{
    log << "===============================================" << '\n';
    log << "                    RESULTS                    " << '\n';
    log << "===============================================" << '\n';

    log << std::setw(15) << "|    Group     " << "|";
    log << std::setw(8)  << " Passed "        << "|";
    log << std::setw(8)  << " Failed "        << "|";
    log << std::setw(12) << "    Time    "      << "|";
    log << '\n';
    log << "-----------------------------------------------" << '\n';

    for (const auto &group: tests) {
        runTests(group.first);
    }

    log << "Passed: " << totalPassed;
    log << ", Failed: " << totalFailed;
    log << ", Time: " << totalTime << "s\n";

    std::ofstream file("results.txt");
    if (file.is_open()) {
        file << log.str();
        if (!failedLog.str().empty()) {
            file << "===============================================" << '\n';
            file << failedLog.str();
        }
        file.close();
    }
}
