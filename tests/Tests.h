#ifndef HK_TESTS_H
#define HK_TESTS_H

#include "hikai.h"

class Tests final : public Application {
public:
    Tests(const AppDesc &desc)
        : Application(desc) {}

    void init();

private:
    void containersTests();
    void mathTests();
};

#endif // HK_TESTS_H
