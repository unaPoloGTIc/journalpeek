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
  Sdj_raii() : tst{"./testdata/"s} {}
  ~Sdj_raii() {}
};

TEST_F(Sdj_raii, ctor) {
  // Make sure that these break the build
  // auto bad1{tst};
  // sd_journal_raii bad2{}; bad2 = tst;
  ASSERT_THROW(sd_journal_raii bad3{"./no/such/dir"};, runtime_error);
  ASSERT_THROW(sd_journal_raii bad4{"./"};, runtime_error); //"Empty journal"
}

class none : public ::testing::Test {
protected:
public:
};

// TODO: replace testdata so that we could test ctor with it.
TEST_F(none, oringFlags) {
  ASSERT_EQ(sdj_opener::flagsToInt({}), 0);
  ASSERT_EQ(sdj_opener::flagsToInt({sdj_opener::openFlags::LOCAL}),
            SD_JOURNAL_LOCAL_ONLY);
  ASSERT_EQ(sdj_opener::flagsToInt({sdj_opener::openFlags::RUNTIME}),
            SD_JOURNAL_RUNTIME_ONLY);
  ASSERT_EQ(sdj_opener::flagsToInt({sdj_opener::openFlags::SYSTEM}),
            SD_JOURNAL_SYSTEM);
  ASSERT_EQ(sdj_opener::flagsToInt({sdj_opener::openFlags::USER}),
            SD_JOURNAL_CURRENT_USER);
  ASSERT_EQ(sdj_opener::flagsToInt({sdj_opener::openFlags::OS_ROOT}),
            SD_JOURNAL_OS_ROOT);

  auto res{sdj_opener::flagsToInt(
      {sdj_opener::openFlags::OS_ROOT, sdj_opener::openFlags::SYSTEM})};
  ASSERT_EQ(res, SD_JOURNAL_OS_ROOT | SD_JOURNAL_SYSTEM);

  ::testing::StaticAssertTypeEq<decltype(res), int>();
}

TEST_F(none, vecstrConv) {
  string s1{"str1"s};
  string s2{"str2"s};
  string s3{"str3"s};
  sdj_opener::vecstrConv conv({s1, s2, s3});
  const char **dest{conv};

  ASSERT_STREQ(s1.c_str(), dest[0]);
  ASSERT_STREQ(s2.c_str(), dest[1]);
  ASSERT_STREQ(s3.c_str(), dest[2]);

  // TODO type assertion for sdj_opener::vecstrConv
}

TEST_F(none, vecstrCtor) {
  sd_journal_wrap tst1{vector<string>{"./testdata/testlog.journal"s}};
}

// To be updated alongside testdata
class Sdj_wrap : public ::testing::Test {
protected:
  sd_journal_wrap tst;

public:
  Sdj_wrap() : tst{"./testdata/"s} {}
  ~Sdj_wrap() {}
};

TEST_F(Sdj_wrap, vecAll) {
  auto v{tst.vec_all()};
  ASSERT_EQ(v.size(), 60);
  ASSERT_EQ(v[0].size(), 29);
  ASSERT_TRUE(any_of(v[0].cbegin(), v[0].cend(), [](auto i) {
    return i == "_BOOT_ID=3ebd06f1166c461bbfcd3028da1cf2c2"s;
  }));
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

TEST_F(Sdj_wrap, vecMsgsFilterMultiple) {
  auto v{tst.vec_msgs("libinput error"s)};
  ASSERT_EQ(v.size(), 6);
}

TEST_F(Sdj_wrap, vecMsgsFilterIgnoreCase) {
  auto v{tst.vec_msgs("sTARTED Network Manager"s, true)};
  ASSERT_EQ(v.size(), 1);
  ASSERT_EQ(v[0], "Started Network Manager Script Dispatcher Service."s);
}

TEST_F(Sdj_wrap, vecMsgsRegexMultiple) {
  auto v{tst.vec_msgs("li[tupnib]{6} error"s)};
  ASSERT_EQ(v.size(), 6);
}

TEST_F(Sdj_wrap, vecMsgsRegexIgnoreCase) {
  auto v{tst.vec_msgs("s[DETRAT]{6} Network Manager"s, true)};
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
  ASSERT_TRUE(any_of(v.cbegin(), v.cend(), [](auto i) {
    return i == "_SOURCE_REALTIME_TIMESTAMP"s;
  }));
}

TEST_F(Sdj_wrap, subjournalSize) {
  int offset{10}, pagesize{10};
  auto v{tst.subJournal(offset, pagesize)};
  ASSERT_EQ(v.size(), pagesize);
}

TEST_F(Sdj_wrap, subjournalContentZerooffset) {
  int offset{0}, pagesize{10};
  auto v{tst.subJournal(offset, pagesize)};
  auto all{tst.vec_msgs()};
  ASSERT_TRUE(equal(v.cbegin(), v.cend(), all.cbegin() + offset));
}

TEST_F(Sdj_wrap, subjournalContentPositiveoffset) {
  int offset{10}, pagesize{10};
  auto v{tst.subJournal(offset, pagesize)};
  auto all{tst.vec_msgs()};
  ASSERT_TRUE(equal(v.cbegin(), v.cend(), all.cbegin() + offset));
}

TEST_F(Sdj_wrap, pagedMsgsSize) {
  int pagesize{10};
  auto [vec, cur, eof] = tst.paged_msgs(""s, pagesize);
  ASSERT_EQ(pagesize, vec.size());
}

TEST_F(Sdj_wrap, pagedMsgsRepeated) {
  int pagesize{10};
  string prevCur{""s};
  for (int i{0}; i < 5; i++) {
    auto [vec, cur, eof] = tst.paged_msgs(prevCur, pagesize);
    prevCur = cur;
    ASSERT_EQ(pagesize, vec.size());
  }
}

TEST_F(Sdj_wrap, pagedMsgsRemainder) {
  int pagesize{35};
  auto [vec1, cur, eof1] = tst.paged_msgs(""s, pagesize);
  ASSERT_EQ(pagesize, vec1.size());
  auto [vec2, unused, eof2] = tst.paged_msgs(cur, pagesize);
  ASSERT_EQ(60 - pagesize, vec2.size());
}

TEST_F(Sdj_wrap, pagedMsgsReverse) {
  int pagesize{10};
  auto [vec1, cur, eof1] = tst.paged_msgs(""s, pagesize);
  auto [vec2, unused, eof2] = tst.paged_msgs(cur, pagesize, "", false, true);
  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_TRUE(equal(vec1.begin() + 1, vec1.end(), vec2.rbegin()));
}

TEST_F(Sdj_wrap, pagedMsgsTinyPage1) {
  int pagesize{1}, prevSize{0};
  string prevCur{""s};
  int i{0};
  auto getEof = [](sd_journal_wrap &tst, string &prevCur, int pagesize,
                   int &i) {
    auto [vec, cur, eof]{tst.paged_msgs(prevCur, pagesize)};
    prevCur = cur;
    i += vec.size();
    return eof;
  };
  while (!getEof(tst, prevCur, pagesize, i)) {
  }
  ASSERT_EQ(i, 60);
}

TEST_F(Sdj_wrap, pagedMsgsTinyPage2) {
  for (int pagesize{1}; pagesize <= 10; pagesize++) {
    sd_journal_wrap tst{"./testdata/"s};
    int prevSize{0};
    string prevCur{""s};
    int i{0};
    bool prevEof;
    do {
      auto [vec, cur, eof] = tst.paged_msgs(prevCur, pagesize);
      prevCur = cur;
      prevSize = vec.size();
      prevEof = eof;
      i += vec.size();
    } while (!prevEof);
    ASSERT_EQ(i, 60);
  }
}

TEST_F(Sdj_wrap, pagedMsgsHugePage) {
  int pagesize{1000};
  auto [vec, cur, eof] = tst.paged_msgs(""s, pagesize);
  ASSERT_EQ(vec.size(), 60);
}

TEST_F(Sdj_wrap, pagedMsgsCursor) {
  int pagesize{5};
  auto [vec, cur, eof] = tst.paged_msgs(""s, pagesize);
  auto [vec1a, cur1a, eof1a] = tst.paged_msgs(cur, pagesize);
  auto [vec1b, cur1b, eof1b] = tst.paged_msgs(cur1a, pagesize);
  auto [vec2, cur2, eof2] = tst.paged_msgs(cur, 2 * pagesize);

  ASSERT_TRUE(equal(vec1a.cbegin(), vec1a.cend(), vec2.cbegin()));
  ASSERT_TRUE(
      equal(vec1b.cbegin(), vec1b.cend(), vec2.cbegin() + vec1a.size()));
}

TEST_F(Sdj_wrap, pagedMsgsFilter) {
  int pagesize{4};

  auto [vec1a, cur1a, eof1a] =
      tst.paged_msgs("", pagesize, "li[tupnib]{6} error"s);
  auto [vec1b, cur1b, eof1b] =
      tst.paged_msgs(cur1a, pagesize, "li[tupnib]{6} error"s);
  auto [vec2, cur2, eof2] =
      tst.paged_msgs("", 2 * pagesize, "li[tupnib]{6} error"s);

  ASSERT_TRUE(equal(vec1a.cbegin(), vec1a.cend(), vec2.cbegin()));
  ASSERT_TRUE(
      equal(vec1b.cbegin(), vec1b.cend(), vec2.cbegin() + vec1a.size()));
}

TEST_F(Sdj_wrap, pagedMsgsFilterCaseInsensitive) {
  int pagesize{4};

  auto [vec1a, cur1a, eof1a] =
      tst.paged_msgs("", pagesize, "li[TUPNIB]{6} error"s, true);
  auto [vec1b, cur1b, eof1b] =
      tst.paged_msgs(cur1a, pagesize, "li[TUPNIB]{6} error"s, true);
  auto [vec2, cur2, eof2] =
      tst.paged_msgs("", 2 * pagesize, "li[TUPNIB]{6} error"s, true);

  ASSERT_TRUE(equal(vec1a.cbegin(), vec1a.cend(), vec2.cbegin()));
  ASSERT_TRUE(
      equal(vec1b.cbegin(), vec1b.cend(), vec2.cbegin() + vec1a.size()));
  ASSERT_TRUE(eof1b);
  ASSERT_TRUE(eof2);
}

TEST_F(Sdj_wrap, pagedMsgsBackwards) {
  int pagesize{10};

  auto [vec1, cur1, eof1] =
      tst.paged_msgs("", pagesize, "li[TUPNIB]{6} error"s, true, false);
  auto [vec2, cur2, eof2] =
      tst.paged_msgs(cur1, pagesize, "li[TUPNIB]{6} error"s, true, true);

  ASSERT_TRUE(equal(vec1.cbegin(), vec1.cend(), vec2.crbegin()));
  ASSERT_TRUE(eof1);
  ASSERT_TRUE(eof2);
}

TEST_F(Sdj_wrap, subjournalMatchesPagedMsgs) {
  int offset{10}, pagesize{10};
  auto v{tst.subJournal(offset, pagesize)};
  auto all{tst.vec_msgs()};
  ASSERT_TRUE(equal(v.cbegin(), v.cend(), all.cbegin() + offset));
}

TEST_F(Sdj_wrap, disjunction) {
  int msgCount{1}, exeCount{8};
  tst.addExactMessageMatch("daemon start"s, "MESSAGE");
  auto v1{tst.vec_msgs()};
  ASSERT_EQ(v1.size(), msgCount);
  tst.removeMatches();

  tst.addExactMessageMatch("/usr/bin/dbus-daemon"s, "_EXE");
  auto v2{tst.vec_msgs()};
  ASSERT_EQ(v2.size(), exeCount);
  tst.removeMatches();

  vector<string> v{"_EXE=/usr/bin/dbus-daemon"s};
  tst.addDisjunction(v);
  tst.addExactMessageMatch("daemon start"s, "MESSAGE");
  auto m{tst.vec_msgs()};
  ASSERT_GE(m.size(), max(exeCount, msgCount)); // TODO: this reveales a bug!
  for (auto i : m)
    cout << i << endl;      // TODO: 'daemon start' shows twice
  cout << m.size() << endl; // TODO: why not exeCount+msgCount
}

TEST_F(Sdj_wrap, conjunction) {
  int msgCount{1}, bootCount{60};
  tst.addExactMessageMatch("daemon start"s, "MESSAGE");
  auto v1{tst.vec_msgs()};
  ASSERT_EQ(v1.size(), msgCount);
  tst.removeMatches();

  tst.addExactMessageMatch("3ebd06f1166c461bbfcd3028da1cf2c2"s, "_BOOT_ID");
  auto v2{tst.vec_msgs()};
  ASSERT_EQ(v2.size(), bootCount);
  tst.removeMatches();

  vector<string> v{"_BOOT_ID=3ebd06f1166c461bbfcd3028da1cf2c2"s};
  tst.addConjunction(v);
  tst.addExactMessageMatch("daemon start"s, "MESSAGE");
  auto m{tst.vec_msgs()};
  ASSERT_EQ(m.size(), min(bootCount, msgCount));
}

class restTester : public ::testing::Test {
protected:
  restServer tst;
  http_client client;

  json::value make_request(string path, json::value const &jvalue) {
    json::value ret;
    client.request(methods::GET, path, jvalue)
        .then([](http_response response) {
          if (response.status_code() == status_codes::OK) {
            return response.extract_json();
          }
          return pplx::task_from_result(json::value());
        })
        .then([&ret](pplx::task<json::value> previousTask) {
          ret = previousTask.get();
        })
        .wait();
    return ret;
  }

public:
  restTester() : tst(), client("http://localhost:6666/") {}
  ~restTester() {}
};

TEST_F(restTester, ctor) {}

TEST_F(restTester, getAll) {
  auto jval = json::value::object();
  auto j{make_request("/v1/paged_search", jval)};
  auto r = j["resp"].as_array();
  ASSERT_EQ(r.size(), 60);
  for (auto &v : r)
    ASSERT_NO_THROW(v.as_string());
}

// TODO: test rest with match, offset, size
// TODO: test primejournal(), cursor(), paged_msgs()

// TODO: unify with others and singletonify
class globalRaii {
public:
  globalRaii() { curl_global_init(CURL_GLOBAL_ALL); }
  ~globalRaii() { curl_global_cleanup(); }
};

int main(int argc, char **argv) {
  globalRaii init{};
  // TODO: generate testdata journal.
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
