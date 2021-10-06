#include <Protocon/Protocon.h>
#include <gtest/gtest.h>

TEST(TestSimple, HelloWorldOk) {
    EXPECT_STREQ("Hello world!", Protocon::HelloWorld());
}
