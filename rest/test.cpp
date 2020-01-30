extern "C" {
#include <curl/curl.h>
}

#include "gtest/gtest.h"
#include "rest-internal.h"
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <iostream>
#include <pplx/pplxtasks.h>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace web;
using namespace web::http;
using namespace web::http::client;

using namespace std;

class Unit : public ::testing::Test {
protected:

public:
  Unit() {}
  ~Unit() {}
};

TEST_F(Unit, ctor) {
  ASSERT_EQ("asd", "asd");
}

//TODO: unify with others and singletonify
class globalRaii {
public:
  globalRaii() { curl_global_init(CURL_GLOBAL_ALL); }
  ~globalRaii() { curl_global_cleanup(); }
};

int main(int argc, char **argv) {
  globalRaii init{};
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
