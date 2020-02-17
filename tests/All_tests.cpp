//
// Created by sebastian on 16/2/20.
//
#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include "Assets and resources.hpp"
#include "ResourceMapTest.hpp"
#define protected public
#define private public
#include "../CRTModel.hpp"

static CRTModel crt;

TEST_CASE("Image selector", "[fileIO]") {
    REQUIRE(crt.loadMedia());
}




