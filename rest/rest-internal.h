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
#include <sstream>
#include <string>

using namespace std;
using handlersMap =
    map<string,function<void(web::http::http_request, web::json::value)>>;

class sdj_opener {
protected:
  sd_journal *raw;//My kingdom for a unique_ptr

public:
  sdj_opener()
  {
    if (sd_journal_open(&raw, 0) < 0)
      throw runtime_error("Can't open default journal");
  }
  sdj_opener(string path)
  {
   if (sd_journal_open_directory(&raw, path.c_str(), 0) < 0)
      throw runtime_error("Can't open journal in path: " + path);
  }

  sdj_opener(const sdj_opener&) = delete;
  sdj_opener& operator=(const sdj_opener&) = delete;

  sdj_opener(sdj_opener&&) = default;
  sdj_opener& operator=(sdj_opener&&) = default;

  virtual ~sdj_opener(){sd_journal_close(raw);};
};

extern string messageLiteral;

class sd_journal_raii: public sdj_opener {
private:
  vector<string> matches;
public:
  void primeJournal()
  {
    if (auto ret{sd_journal_seek_head(raw)}; ret < 0)
      throw runtime_error("Failed to reset to journal head");
    if (auto ret{sd_journal_next(raw)}; ret == 0)
      throw runtime_error("Empty journal");
    else if (ret < 0)
      throw runtime_error("Error during first read");
  }

  sd_journal_raii():sdj_opener()
  {
    primeJournal();
  }
  sd_journal_raii(string path):sdj_opener(path)
  {
    primeJournal();
  }

  sd_journal_raii(const sd_journal_raii&) = delete;
  sd_journal_raii& operator=(const sd_journal_raii&) = delete;

  sd_journal_raii(sd_journal_raii&&) = default;
  sd_journal_raii& operator=(sd_journal_raii&&) = default;

  operator sd_journal*(){return raw;}

  void addExactMatch(string text, string field = messageLiteral)
  {
    //TODO: not needed if sd_journal copies.
    auto tmp{matches.emplace_back(field + "="s + text)};
    sd_journal_add_match(raw, tmp.c_str(), 0);
    primeJournal();
  }

  void removeMatches()
  {
    sd_journal_flush_matches(raw);
    matches.clear();
  }
};

class sd_journal_wrap {
private:
  sd_journal_raii journal;

  using recordCallback = function<void(sd_journal_raii&)>;
  void recordIterator(recordCallback rcb)
  {
    SD_JOURNAL_FOREACH(journal)
      {
	rcb(journal);
      }
  }

public:
  sd_journal_wrap():journal{}{};
  sd_journal_wrap(string path):journal{path}{};

  sd_journal_wrap(const sd_journal_wrap&) = delete;
  sd_journal_wrap& operator=(const sd_journal_wrap&) = delete;

  sd_journal_wrap(sd_journal_wrap&&) = default;
  sd_journal_wrap& operator=(sd_journal_wrap&&) = default;

  vector<vector<string>> vec_all()
  {
    vector<vector<string>> ret;
    journal.primeJournal();
    auto recHandler = [&ret](sd_journal_raii& journal)
		      {
			const void *data;
			size_t length;
			vector<string> rec;
			SD_JOURNAL_FOREACH_DATA(journal, data, length)
			  {
			    rec.emplace_back(static_cast<const char*>(data));
			  }
			ret.push_back(move(rec));
		      };
    recordIterator(recHandler);
    journal.primeJournal();
    return ret;//moves
  }

  //TODO: overload for regex
  vector<string> vec_msgs(string filter = ""s, bool ignoreCase = false)
  {
    vector<string> ret;
    journal.primeJournal();
    auto recHandler = [&ret, filter, ignoreCase](sd_journal_raii& journal)
		      {
			const char *d;
			size_t l;
			if (auto r = sd_journal_get_data(journal,
							 messageLiteral.c_str(),
							 (const void **)&d,
							 &l); r < 0)
			  {
			    cerr << "Failed to read message field: " << string{strerror(-r)} << endl;
			    return;
			  }

			string msg{d};
			//remove field name
			msg = msg.substr(msg.find('=')+1, msg.npos);
			string msgCpy{msg}, filterCpy{filter};
			if (ignoreCase)
			  {
			    auto lowerize = [](auto& i){i = tolower(i);};
			    for_each(msg.begin(), msg.end(), lowerize);
			    for_each(filterCpy.begin(), filterCpy.end(), lowerize);
			  }
			if (msg.find(filterCpy) != msg.npos)
			  ret.push_back(move(msgCpy));
		      };
    recordIterator(recHandler);
    journal.primeJournal();
    return ret;//moves
  }

  vector<string> fieldnames()
  {
    const char *field;
    vector<string> ret;
    SD_JOURNAL_FOREACH_FIELD(journal, field)
      ret.emplace_back(field);
    sd_journal_restart_fields(journal);
    return ret;
  }

  void addExactMessageMatch(string text)
  {
    journal.addExactMatch(text);
  }

  void removeMatches()
  {
    journal.removeMatches();
  }
};

class restServer {
private:
  web::http::experimental::listener::http_listener s;

public:
  ~restServer();

  static bool verifyToken(
      string token) //TODO AF
  {
    return true;
  }

  restServer(handlersMap endpoints, string port);
};
