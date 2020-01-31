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

class Sdj_raii : public ::testing::Test {
protected:
  sd_journal_raii tst;
public:
  Sdj_raii():tst{"./testdata/"s}{}
  ~Sdj_raii() {}
};

TEST_F(Sdj_raii, ctor) {
  //Make sure that these break the build
  //auto bad1{tst};
  //sd_journal_raii bad2{}; bad2 = tst;
  ASSERT_THROW(sd_journal_raii bad3{"./no/such/dir"};, runtime_error);
  ASSERT_THROW(sd_journal_raii bad4{"./"};, runtime_error);//"Empty journal"
}

class Sdj_wrap : public ::testing::Test {
protected:
  sd_journal_wrap tst;
public:
  Sdj_wrap():tst{"./testdata/"s}{}
  ~Sdj_wrap() {}
};

TEST_F(Sdj_wrap, vecAll) {
  auto v{tst.vec_all()};
  //To be updated alongside testdata
  ASSERT_EQ(v.size(), 60);
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
