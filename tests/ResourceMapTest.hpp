#include <Asset.h>

void ResourceAppend(ResourceMap& res, std::string str) {
    Channel chn;
    chn.SetName(str);
    res[res.size()] = chn;
}


TEST_CASE("Resource Map", "[Core]") {
    ResourceMap res;

    SECTION("Initialization, size query") {
        REQUIRE(res.size() == 0);
    }

    SECTION("Insert and Query by key") {
        Channel element;
        ResourceAppend(res, "Hello first!");
        REQUIRE(res.size() == 1);
        ResourceAppend(res, "Hello second!");
        REQUIRE(res.size() == 2);
        REQUIRE(res.at(0).GetName() == "Hello first!");
        REQUIRE(res.at(1).GetName() == "Hello second!");
        ResourceAppend(res, "This is going wild!");
        REQUIRE(res.size() == 3);
        REQUIRE(res[2].GetName() == "This is going wild!");
    }

    SECTION("Iterator Tests") {
        int count = 0;
        ResourceAppend(res, "This is ");
        ResourceAppend(res, "an Iterator");
        ResourceAppend(res, "Cake Day!!");
        for(auto& x : res) {
            std::cout << x.first << ", " << x.second.GetName() << std::endl;
            ++count;
        }
        REQUIRE(count == res.size());
    }


    SECTION("Delete Elements") {
        ResourceAppend(res, "This is ");
        ResourceAppend(res, "an Iterator");
        ResourceAppend(res, "Cake Day!!");

        REQUIRE(res.size() == 3);
        res.erase(0);
        REQUIRE(res.size() == 2);
        res.erase(1);
        REQUIRE(res.size() == 1);
        REQUIRE(res.at(2).GetName() == "Cake Day!!");
        res.clear();
        REQUIRE(res.size() == 0);
    }



}
