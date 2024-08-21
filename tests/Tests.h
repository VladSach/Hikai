#ifndef HK_TESTS_H
#define HK_TESTS_H

#include "hikai.h"

class Tests final : public Application {
public:
    Tests(const AppDesc &desc)
        : Application(desc) {}

    void init();

private:
    void mathTests();

    // Utils
    void containersTests();
    void numericsTests();
};

#endif // HK_TESTS_H
