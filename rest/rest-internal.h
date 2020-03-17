extern "C" {
#include <systemd/sd-journal.h>
}
#include <algorithm>
#include <array>
#include <chrono>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <pplx/pplxtasks.h>
#include <random>
#include <regex>
#include <sstream>
#include <string>

using namespace std;
using handlersMap =
    map<string, function<void(web::http::http_request, web::json::value)>>;

class sdj_opener {
protected:
public:
  sd_journal *raw; // My kingdom for a unique_ptr

  using vecstr = vector<string>;
  class vecstrConv final {
  private:
    vecstr src;
    const char **data;

  public:
    vecstrConv(vecstr &&d) : src{d} { data = new const char *[src.size() + 1]; }
    vecstrConv(const vecstrConv &) = delete;
    vecstrConv &operator=(const vecstrConv &) = delete;

    vecstrConv(vecstrConv &&) = default;
    vecstrConv &operator=(vecstrConv &&) = default;
    ~vecstrConv() { delete[] data; }

    operator const char **() {
      const char **iter{data};
      for (auto &i : src)
        *iter++ = i.c_str();
      *iter = nullptr;
      return data;
    }
  };

  enum class openFlags : int {

    LOCAL = SD_JOURNAL_LOCAL_ONLY,
    RUNTIME = SD_JOURNAL_RUNTIME_ONLY,
    SYSTEM = SD_JOURNAL_SYSTEM,
    USER = SD_JOURNAL_CURRENT_USER,
    OS_ROOT = SD_JOURNAL_OS_ROOT,
    // Not yet available in debian testing
    // ALL_NAMESPACES = SD_JOURNAL_ALL_NAMESPACES,
    // INCLUDE_DEFAULT_NAMESPACE = SD_JOURNAL_INCLUDE_DEFAULT_NAMESPACE,
  };

  using flagList = initializer_list<openFlags>;

  static int flagsToInt(flagList l) {
    int ret = 0;
    for (auto f : l)
      ret |= static_cast<int>(f);
    return ret;
  }

  sdj_opener(int flags = 0) {
    if (sd_journal_open(&raw, flags) < 0)
      throw runtime_error("Can't open default journal");
  }
  sdj_opener(flagList fl) : sdj_opener{flagsToInt(fl)} {}

  sdj_opener(string path, int flags = 0) {
    if (sd_journal_open_directory(&raw, path.c_str(), flags) < 0)
      throw runtime_error("Can't open journal in path: " + path);
  }
  sdj_opener(string path, flagList fl) : sdj_opener{path, flagsToInt(fl)} {}

  sdj_opener(vecstr &&paths, int flags = 0) {
    if (sd_journal_open_files(&raw, vecstrConv{move(paths)}, flags) < 0)
      throw runtime_error("Can't open journal files");
  }
  sdj_opener(vecstr &&paths, flagList fl)
      : sdj_opener{move(paths), flagsToInt(fl)} {}

  sdj_opener(const sdj_opener &) = delete;
  sdj_opener &operator=(const sdj_opener &) = delete;

  sdj_opener(sdj_opener &&) = default;
  sdj_opener &operator=(sdj_opener &&) = default;

  virtual ~sdj_opener() { sd_journal_close(raw); };
};

extern string messageLiteral;

class sd_journal_raii : public sdj_opener {
  string headCur;
  string tailCur;

  void addLogicalMatchers(const vector<string> &terms,
                          function<int(sd_journal *)> delegate) {
    for (auto &i : terms) {
      if (sd_journal_add_match(raw, i.c_str(), 0) < 0)
        throw runtime_error("Could not add match: " + i);
    }
    if (delegate(raw) < 0)
      throw runtime_error("Could not delegate to logical matcher");
    primeJournal();
  }

  const string
  getHead() { // TODO: consider saving and restoring current position
    if (int ret{sd_journal_seek_head(raw)}; ret < 0)
      throw runtime_error("Failed to seek head cursor: "s +
                          string{strerror(-ret)});
    if (auto ret{sd_journal_next(raw)}; ret < 1)
      throw runtime_error("Journal empty? "s + string{strerror(-ret)});
    return cursor();
  }
  const string getTail() { // TODO: unify with getHead()
    if (int ret{sd_journal_seek_tail(raw)}; ret < 0)
      throw runtime_error("Failed to seek tail cursor: "s +
                          string{strerror(-ret)});
    if (auto ret{sd_journal_previous(raw)}; ret < 1)
      throw runtime_error("Journal empty? "s + string{strerror(-ret)});
    return cursor();
  }

public:
  void primeJournal() { // disambiguate
    primeJournal(1);
  }

  void primeJournal(unsigned int offset) {
    if (auto ret{sd_journal_seek_head(raw)}; ret < 0)
      throw runtime_error("Failed to reset to journal head");
    if (auto ret{sd_journal_next_skip(raw, offset)};
        ret >= 0 && ret < static_cast<int>(offset))
      throw runtime_error("Journal empty or shorter than offset");
    else if (ret < 0)
      throw runtime_error("Error during first read");
  }

  void primeJournalEnd() { // TODO:unify
    if (auto ret{sd_journal_seek_tail(raw)}; ret < 0)
      throw runtime_error("Failed to reset to journal tail");
    if (auto ret{sd_journal_previous(raw)}; ret == 0)
      throw runtime_error("Journal empty");
    else if (ret < 0)
      throw runtime_error("Error during first read");
  }

  void primeJournal(const string &cursor, bool backwards) {
    if (cursor == ""s) {
      if (backwards)
        return primeJournalEnd();
      else
        return primeJournal();
    }
    if (auto ret{sd_journal_seek_cursor(raw, cursor.c_str())}; ret < 0)
      throw runtime_error("Failed to reset journal to cursor");

    if (backwards) // either success or EOF, no error.
      sd_journal_previous(raw);
    else
      sd_journal_next(raw);
  }

  string cursor() {
    char *d;
    string tmp{};
    auto ret{sd_journal_get_cursor(raw, &d)};

    if (ret < 0) {
      if (ret == -ENOMEM)
        throw runtime_error(
            "Failed to get cursor, don't know if need to free()");
      throw runtime_error("Failed to get cursor");
    }

    try {
      tmp = string{d};
    } catch (...) {
      free(d); // TODO: add raii object
      throw;
    }
    free(d);
    return tmp;
  }

  bool testCursor(string cur) {
    if (int ret{sd_journal_test_cursor(raw, cur.c_str())}; ret < 0)
      throw runtime_error("Failed to compare cursors: "s +
                          string{strerror(-ret)});
    else
      return ret > 0;
  }

  sd_journal_raii(int flags = 0)
      : sdj_opener(flags), headCur{getHead()}, tailCur{getTail()} {
    primeJournal();
  }
  sd_journal_raii(sdj_opener::flagList fl)
      : sdj_opener(fl), headCur{getHead()}, tailCur{getTail()} {
    primeJournal();
  }
  sd_journal_raii(string path, int flags = 0)
      : sdj_opener(path, flags), headCur{getHead()}, tailCur{getTail()} {
    primeJournal();
  }
  sd_journal_raii(string path, sdj_opener::flagList fl)
      : sdj_opener(path, fl), headCur{getHead()}, tailCur{getTail()} {
    primeJournal();
  }

  sd_journal_raii(sdj_opener::vecstr &&paths, int flags = 0)
      : sdj_opener(move(paths), flags), headCur{getHead()}, tailCur{getTail()} {
    primeJournal();
  }

  sd_journal_raii(sdj_opener::vecstr &&paths, sdj_opener::flagList fl)
      : sdj_opener(move(paths), fl), headCur{getHead()}, tailCur{getTail()} {
    primeJournal();
  }

  sd_journal_raii(const sd_journal_raii &) = delete;
  sd_journal_raii &operator=(const sd_journal_raii &) = delete;

  sd_journal_raii(sd_journal_raii &&) = default;
  sd_journal_raii &operator=(sd_journal_raii &&) = default;

  operator sd_journal *() { return raw; }

  void addExactMatch(string text, string field = messageLiteral) {
    string tmp{field + "="s + text};
    if (sd_journal_add_match(raw, tmp.c_str(), 0) < 0)
      throw runtime_error("Failed to add match: " + tmp);
    primeJournal();
  }

  void removeMatches() { sd_journal_flush_matches(raw); }

  void addDisjunction(const vector<string> &terms) {
    try {
      addLogicalMatchers(terms, sd_journal_add_disjunction);
    } catch (runtime_error &e) {
      throw runtime_error("failed to add disjunction: "s + string{e.what()});
    }
  }

  void addConjunction(const vector<string> &terms) {
    try {
      addLogicalMatchers(terms, sd_journal_add_conjunction);
    } catch (runtime_error &e) {
      throw runtime_error("failed to add conjunction: "s + string{e.what()});
    }
  }

  string headCursor() { return headCur; }
  string tailCursor() { return tailCur; }
};

class sd_journal_wrap { // TODO: add iterator, stream.
private:
  sd_journal_raii journal;

  using recordCallback = function<void(sd_journal_raii &)>;
  void recordIterator(recordCallback rcb) {
    SD_JOURNAL_FOREACH(journal) { rcb(journal); }
  }

  string get_current_msg() {
    const char *d;
    size_t l;
    if (auto r = sd_journal_get_data(journal, messageLiteral.c_str(),
                                     reinterpret_cast<const void **>(&d), &l);
        r < 0) {
      return "Failed to read message: "s + string{strerror(-r)} + "\n";
    }

    string msg{d};
    removeFieldName(msg);
    return msg;
  }

  using msgCallback = function<bool(const string &)>;
  using advance_type = function<int(sd_journal *)>;

  tuple<string, bool> find_if_not(string begin, msgCallback mcb,
                                  bool backwards = false) {
    journal.primeJournal(begin, backwards);

    int advanceTmp;
    bool cbTmp;
    auto advance{backwards ? sd_journal_previous : sd_journal_next};
    while (cbTmp = mcb(get_current_msg()), advanceTmp = advance(journal),
           advanceTmp && cbTmp) {
    }

    return make_tuple(journal.cursor(), !advanceTmp);
  }

  function<regex(string, bool)> regWrap{[](string filter, bool ignoreCase) {
    auto opts{regex_constants::ECMAScript | regex_constants::optimize};
    if (ignoreCase)
      opts |= regex_constants::icase;
    return regex{filter, opts};
  }};

public:
  sd_journal_wrap(int flags = 0) : journal{flags} {};
  sd_journal_wrap(string path, int flags = 0) : journal{path, flags} {};
  sd_journal_wrap(sdj_opener::vecstr &&paths, int flags = 0)
      : journal{move(paths), flags} {};

  sd_journal_wrap(sdj_opener::flagList fl) : journal{fl} {};
  sd_journal_wrap(string path, sdj_opener::flagList fl) : journal{path, fl} {};
  sd_journal_wrap(sdj_opener::vecstr &&paths, sdj_opener::flagList fl)
      : journal{move(paths), fl} {};

  sd_journal_wrap(const sd_journal_wrap &) = delete;
  sd_journal_wrap &operator=(const sd_journal_wrap &) = delete;

  sd_journal_wrap(sd_journal_wrap &&) = default;
  sd_journal_wrap &operator=(sd_journal_wrap &&) = default;

  vector<vector<string>> vec_all() {
    vector<vector<string>> ret;
    journal.primeJournal();
    auto recHandler = [&ret](sd_journal_raii &journal) {
      const void *data;
      size_t length;
      vector<string> rec;
      SD_JOURNAL_FOREACH_DATA(journal, data, length) {
        rec.emplace_back(static_cast<const char *>(data));
      }
      ret.push_back(move(rec));
    };
    recordIterator(recHandler);
    journal.primeJournal();
    return ret; // moves
  }

  static void removeFieldName(string &msg) {
    msg = msg.substr(msg.find('=') + 1, msg.npos);
  }

  vector<string> vec_msgs(string filter = ""s,
                          bool ignoreCase = false) { // TODO: page-ify
    vector<string> ret;
    auto reg{regWrap(filter, ignoreCase)};

    auto regHandler = [&ret, &reg](sd_journal_raii &journal) {
      // TODO: use get_current_msg() instead
      const char *d;
      size_t l;
      if (auto r = sd_journal_get_data(journal, messageLiteral.c_str(),
                                       reinterpret_cast<const void **>(&d), &l);
          r < 0) {
        cerr << "Failed to read message field: " << string{strerror(-r)}
             << endl;
        return;
      }

      string msg{d};
      removeFieldName(msg);
      if (regex_search(msg, reg)) {
        ret.push_back(msg);
      }
    };

    recordIterator(regHandler);
    journal.primeJournal();
    return ret; // moves
  }

  tuple<vector<string>, string, bool>
  paged_msgs(string begin = ""s, long unsigned int pagesize = 100,
             string filter = ""s, bool ignoreCase = false,
             bool backwards = false) {
    vector<string> ret_vec{};
    auto reg{regWrap(filter, ignoreCase)};

    auto countingMsgHandler = [&ret_vec, &reg, pagesize](const string &msg) {
      if (regex_search(msg, reg))
        ret_vec.push_back(msg);

      return ret_vec.size() < pagesize;
    };

    if (pagesize == 0)
      return make_tuple(ret_vec, begin, journal.testCursor(begin));

    auto [end, eof] = this->find_if_not(begin, countingMsgHandler, backwards);
    return make_tuple(ret_vec, end, eof);
  }

  vector<string> fieldnames() {
    const char *field;
    vector<string> ret;
    SD_JOURNAL_FOREACH_FIELD(journal, field)
    ret.emplace_back(field);
    sd_journal_restart_fields(journal);
    return ret;
  }

  vector<string> fieldUnique(string field) {
    vector<string> ret;
    const void *d;
    size_t l;
    if (auto r = sd_journal_query_unique(journal, field.c_str()); r < 0)
      throw runtime_error("Failed to query for field: "s + field + " : "s +
                          string{strerror(-r)});
    SD_JOURNAL_FOREACH_UNIQUE(journal, d, l)
    ret.emplace_back(static_cast<const char *>(d));
    return ret;
  }

  void addExactMessageMatch(string text, string field = messageLiteral) {
    journal.addExactMatch(text, field);
  }

  void removeMatches() { journal.removeMatches(); }

  vector<string> subJournal(int offset, int pagesize = 100) {
    vector<string> ret;
    auto getOne{[](sd_journal_raii &journal, vector<string> &ret) {
      // TODO: use get_current_msg() instead
      const char *d;
      size_t l;
      int get{sd_journal_get_data(journal, messageLiteral.c_str(),
                                  reinterpret_cast<const void **>(&d), &l)};
      removeFieldName(ret.emplace_back(d));
      int next{sd_journal_next(journal)};
      return get == 0 && next > 0;
    }};

    journal.primeJournal(offset + 1);
    for (; pagesize > 0 && getOne(journal, ret); pagesize--) {
    }
    journal.primeJournal();
    return ret;
  }

  void addDisjunction(const vector<string> &terms) {
    journal.addDisjunction(terms);
  }

  void addConjunction(const vector<string> &terms) {
    journal.addConjunction(terms);
  }
};

extern handlersMap jdwrapper;

class httpsConfWrapper {
  web::http::experimental::listener::http_listener_config lc;
  function<void(boost::asio::ssl::context &)> sslCbWrapper;

public:
  httpsConfWrapper()
      : lc{}, sslCbWrapper{[](boost::asio::ssl::context &sc) {
          sc.set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2);
          sc.use_certificate_file("/certs/live/trex-security.com/fullchain.pem",
                                  boost::asio::ssl::context::pem);
          sc.use_private_key_file("/certs/live/trex-security.com/privkey.pem",
                                  boost::asio::ssl::context::pem);
          return;
        }} {
    lc.set_ssl_context_callback(sslCbWrapper);
  }
  const web::http::experimental::listener::http_listener_config &get() {
    return lc;
  }
};

class restServer {
private:
  httpsConfWrapper cw;
  web::http::experimental::listener::http_listener s;

public:
  ~restServer();

  restServer(handlersMap endpoints = jdwrapper, string proto = "http"s);
};
