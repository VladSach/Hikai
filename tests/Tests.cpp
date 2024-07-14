#include "Tests.h"

#include "UnitTest.h"

void Tests::init()
{
    containersTests();
    mathTests();

    RUN_ALL_TESTS();

    EventSystem::instance()->fireEvent(hk::EVENT_APP_SHUTDOWN, {});
}

void Tests::containersTests()
{
    struct Mock {
        u32 id;
        f32 dummy;
    };

    static hk::vector<Mock> hkvec(4, {80085, 11.f});
    static hk::ring_buffer<Mock, 10> hkring;

    DEFINE_TEST("Containers", "Vector default value constructor",
    {
        for (u32 i = 0; i < hkvec.size()-1; i++) {
            EXPECT_EQ(hkvec[i].id, hkvec[i+1].id);
            EXPECT_EQ(hkvec[i].dummy, hkvec[i+1].dummy);
        }
    });

    DEFINE_TEST("Containers", "Vector copy constructor",
    {
        hk::vector<Mock> copy(hkvec);
        for (u32 i = 0; i < hkvec.size(); i++) {
            EXPECT_EQ(hkvec[i].id, copy[i].id);
            EXPECT_EQ(hkvec[i].dummy, copy[i].dummy);
        }
    });

    DEFINE_TEST("Containers", "Vector operations",
    {
        hkvec.resize(10);
        hkvec.clear();

        EXPECT_EQ(hkvec.size(), (u32)0);
        EXPECT_EQ(hkvec.capacity(), (u32)10);

        hkvec.push_back({1, 0});
        hkvec.push_back({10, 0});
        hkvec.push_back({100, 0});

        EXPECT_EQ(hkvec.size(), (u32)3);

        EXPECT_EQ(hkvec[0].id, (u32)1);
        EXPECT_EQ(hkvec[1].id, (u32)10);
        EXPECT_EQ(hkvec[2].id, (u32)100);

        hkvec.pop_back();
        EXPECT_EQ(hkvec.size(), (u32)2);

        hkvec.resize(1);
        hkvec.pop_back();
        EXPECT_EQ(hkvec.empty(), true);

        hkvec.push_back({1, 0});
        hkvec.push_back({2, 0});
        hkvec.erase(hkvec.begin());
        EXPECT_EQ(hkvec.at(0).id, (u32)2);
    });

    DEFINE_TEST("Containers", "Ring Buffer operations",
    {
        for(u32 i = 0; hkring.push({i, 0}) && i < 15; ++i) {}

        Mock value; hkring.peek(value);
        EXPECT_EQ(value.id, (u32)0);

        for(u32 i = 0; hkring.pop(value); ++i) {}

        hkring.push({1337, 0});
        hkring.push({80085, 0});

        EXPECT_EQ(hkring[0].id, (u32)1337);
        EXPECT_EQ(hkring[1].id, (u32)80085);

        hkring.clear();
        EXPECT_EQ(hkring.size(), (u32)0);
    });
}

void Tests::mathTests()
{
    constexpr f32 epsilon = std::numeric_limits<float>::epsilon();

    // TODO: expend vec2f and vec3f tests to other operations
    DEFINE_TEST("Math", "vec2f operations",
    {
        hkm::vec2f a(0.4f, 0.22f);
        hkm::vec2f b(0.64f, -0.92f);

        f32 dot = hkm::dot(a, b);

        constexpr f32 expectedDot = .0536f;
        f32 diff = expectedDot - dot;
        b8 result = (diff < epsilon) && (-diff < epsilon);

        EXPECT_EQ(result, true);
    });

    DEFINE_TEST("Math", "vec3f operations",
    {
        hkm::vec3f a(.4f, .22f, .37f);
        hkm::vec3f b(.64f, .92f, .02f);

        f32 dot = hkm::dot(a, b);

        constexpr f32 expectedDot = .4658f;
        f32 diff = expectedDot - dot;
        b8 result = (diff < epsilon) && (-diff < epsilon);

        EXPECT_EQ(result, true);
    });
}
