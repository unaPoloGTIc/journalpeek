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

//To be updated alongside testdata
class Sdj_wrap : public ::testing::Test {
protected:
  sd_journal_wrap tst;
public:
  Sdj_wrap():tst{"./testdata/"s}{}
  ~Sdj_wrap() {}
};

TEST_F(Sdj_wrap, vecAll) {
  auto v{tst.vec_all()};
  ASSERT_EQ(v.size(), 60);
  ASSERT_EQ(v[0].size(), 29);
  ASSERT_TRUE(any_of(v[0].cbegin(),
		     v[0].cend(),
		     [](auto i)
		     {return i=="_BOOT_ID=3ebd06f1166c461bbfcd3028da1cf2c2"s;}));
}

TEST_F(Sdj_wrap, vecMsgs) {
  auto v{tst.vec_msgs()};
  ASSERT_EQ(v.size(), 60);
  ASSERT_EQ(v[0], "Started Network Manager Script Dispatcher Service."s);
}

TEST_F(Sdj_wrap, vecMsgsFilterCaseful) {
  auto v{tst.vec_msgs("Started Network Manager"s)};
  ASSERT_EQ(v.size(), 1);
  ASSERT_EQ(v[0], "Started Network Manager Script Dispatcher Service."s);
}

TEST_F(Sdj_wrap, vecMsgsFilterIgnoreCase) {
  auto v{tst.vec_msgs("sTARTED Network Manager"s, true)};
  ASSERT_EQ(v.size(), 1);
  ASSERT_EQ(v[0], "Started Network Manager Script Dispatcher Service."s);
}

TEST_F(Sdj_wrap, vecMsgsResetAfterMatch) {
  auto v{tst.vec_msgs()};
  tst.addExactMessageMatch("daemon start"s);
  ASSERT_NO_THROW(auto v2{tst.vec_msgs()});
}

TEST_F(Sdj_wrap, exactMatch) {
  tst.addExactMessageMatch("daemon start"s);
  auto v{tst.vec_msgs()};
  ASSERT_EQ(v.size(), 1);
}

TEST_F(Sdj_wrap, resetMatch) {
  tst.addExactMessageMatch("daemon start"s);
  auto v{tst.vec_msgs()};
  tst.removeMatches();
  auto v2{tst.vec_msgs()};
  ASSERT_EQ(v2.size(), 60);
}

TEST_F(Sdj_wrap, fields) {
  auto v{tst.fieldnames()};
  ASSERT_EQ(v.size(), 42);
  ASSERT_TRUE(any_of(v.cbegin(),
		     v.cend(),
		     [](auto i)
		     {return i=="_SOURCE_REALTIME_TIMESTAMP"s;}));
}

//TODO: unify with others and singletonify
class globalRaii {
public:
  globalRaii() { curl_global_init(CURL_GLOBAL_ALL); }
  ~globalRaii() { curl_global_cleanup(); }
};

int main(int argc, char **argv) {
  globalRaii init{};
  //TODO: generate testdata journal.
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
