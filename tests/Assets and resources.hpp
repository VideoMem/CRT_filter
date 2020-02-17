#include <Asset.h>

TEST_CASE("Assets & Resources", "[CPU Memory Ops][Core]") {
    SECTION("Assets") {
        Asset<std::string> test;
        test.Set("Hello World");
        REQUIRE(test.Get() == "Hello World");
        Asset<int> iTest;
        iTest.Set(42);
        REQUIRE(iTest.Get() == 42);
    }

    SECTION("Resources") {
        Channel res;
        res.SetName("Resource Name");
        REQUIRE(res.GetName() == "Resource Name");
        res.SetType("image");
        REQUIRE(res.GetType() == "image");
        res.SetUri("uri//uri");
        REQUIRE(res.GetUri() == "uri//uri");
    }

}




