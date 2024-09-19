#include "Tests.h"

#include "UnitTest.h"

void Tests::init()
{
    containersTests();
    mathTests();
    numericsTests();
    stringsTests();

    RUN_ALL_TESTS();

    EventSystem::instance()->fireEvent(hk::EVENT_APP_SHUTDOWN, {});
}

// TODO: move this function out of here
// Function to calculate power of 10 for given precision
f32 pow10(u32 precision) {
    f32 result = 1.0f;
    for(u32 i = 0; i < precision; ++i) {
        result *= 10.0f;
    }
    return result;
}

// Generalized rounding function adjustable to any desired number of decimal places
f32 roundToPrecision(f32 value, u32 numDecimalPlaces) {
    f32 multiplier = pow10(numDecimalPlaces);

    b8 isNegative = value < 0;
    f32 absValue = isNegative ? -value : value;

    f32 multiplied = absValue * multiplier;
    f32 truncated = static_cast<f32>(static_cast<u32>(multiplied + 0.5f));
    f32 rounded = truncated / multiplier;

    // Correctly handle very small numbers close to zero
    if ((multiplied + 0.5f) < truncated) {
        rounded -= 1.0f / multiplier;
    }

    return isNegative ? -rounded : rounded;
}

// Function to round all elements of mat3f to a specified precision
hkm::mat3f roundMat3f(const hkm::mat3f &matrix, u32 precision) {
    hkm::mat3f out;
    for(u32 i = 0; i < 3; ++i) {
        for(u32 j = 0; j < 3; ++j) {
            out.n[i][j] = roundToPrecision(matrix.n[i][j], precision);
        }
    }
    return out;
}

hkm::quaternion roundQuat(const hkm::quaternion &quat, u32 precision) {
    hkm::quaternion out;
    out.x = roundToPrecision(quat.x, precision);
    out.y = roundToPrecision(quat.y, precision);
    out.z = roundToPrecision(quat.z, precision);
    out.w = roundToPrecision(quat.w, precision);
    return out;
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
    auto epsilon = [](f32 a, f32 b){
        return 1.0e-5f * fmax(fabs(a), fabs(b));
    };

    // TODO: expend vec2f and vec3f tests to other operations
    DEFINE_TEST("Math", "vec2f operations",
    {
        hkm::vec2f a(0.4f, 0.22f);
        hkm::vec2f b(0.64f, -0.92f);

        f32 dot = hkm::dot(a, b);

        constexpr f32 expectedDot = .0536f;
        f32 diff = expectedDot - dot;
        f32 eps = epsilon(dot, expectedDot);
        b8 result = (diff < eps) && (-diff < eps);

        EXPECT_EQ(result, true);
    });

    DEFINE_TEST("Math", "vec3f operations",
    {
        constexpr hkm::vec3f a(.4f, .22f, .37f);
        constexpr hkm::vec3f b(.64f, .92f, .02f);

        constexpr f32 dot = hkm::dot(a, b);

        constexpr f32 expectedDot = .4658f;
        f32 diff = fabs(expectedDot - dot);
        b8 result = diff < epsilon(dot, expectedDot);

        EXPECT_EQ(result, true);
    });

    DEFINE_TEST("Math", "mat4f operations",
    {
        hkm::mat4f a({
            0.f, 0.f, -1.f, 2.f,
            0.f, 1.f, 0.f, 0.f,
            9.f, 0.f, 0.f, 0.f,
            0.f, 0.f, 0.f, 1.f,
        });

        hkm::mat4f expectedInverse({
            0.f,  0.f, 1.f/9.f, 0.f,
            0.f,  1.f, 0.f, 0.f,
            -1.f, 0.f, 0.f, 2.f,
            0.f,  0.f, 0.f, 1.f,
        });

        hkm::mat4f inverse = hkm::inverse(a);

        EXPECT_EQ(inverse, expectedInverse);
    });

    DEFINE_TEST("Math", "quaternion operations",
    {
        hkm::quaternion quat;
        hkm::quaternion expectedQuat;

        quat = hkm::quaternion({1.4897f,  1.409f,  1.4172f, -0.12414f});
        hkm::quaternion quat2({ 0.31877f, 3.5784f, 0.7254f,  0.53767f});

        quat = quat * quat2;
        expectedQuat = hkm::quaternion({4.8106f, 0.9422f, -4.2097f, -6.6116f});
        EXPECT_EQ(roundQuat(quat, 4), expectedQuat);

        quat = quat * hkm::quaternion::identity();
        EXPECT_EQ(roundQuat(quat, 4), expectedQuat);

        // quat = hkm::fromEulerAngles({30, 45, 0});
        // expectedQuat = hkm::quaternion(0.23912f, 0.36964f, 0.099046f, 0.8924f);
        // EXPECT_EQ(quat, expectedQuat);

        quat = hkm::fromAxisAngle({1.f, 0.f, 0.f}, hkm::pi/2);
        expectedQuat = hkm::quaternion(0.7071f, 0.f, 0.f, 0.7071f);
        EXPECT_EQ(roundQuat(quat, 4), expectedQuat);

        quat = hkm::fromAxisAngle({0.2f, 5.3f, -1.4f}, 0.f);
        EXPECT_EQ(roundQuat(quat, 4), hkm::quaternion::identity());

        quat = hkm::quaternion(0.23912f, 0.36964f, 0.099046f, 0.8924f);
        hkm::mat3f rot = quat.rotmat();
        hkm::mat3f expectedRot({
             0.7071f, -0.f,      0.7071f,
             0.3536f,  0.8660f, -0.3536f,
            -0.6124f,  0.5f,     0.6124f,
        });

        EXPECT_EQ(roundMat3f(rot, 4), expectedRot);
    });
}

void Tests::numericsTests()
{
    DEFINE_TEST("Numerics", "Random", {
        hk::xoshiro256ss rng;

        for (u32 i = 0; i < 10; ++i) {
            // TODO: How to even test random?
            // EXPECT_EQ(rng(), 0ULL);
        }
    });
}

void Tests::stringsTests()
{
    DEFINE_TEST("Strings", "Locale", {
        std::wstring wstr(L"TestString.str");
        std::string str("TestString.str");

        EXPECT_EQ(str, hk::wstring_convert(wstr));
    });
}
