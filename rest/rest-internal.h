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

class sd_journal_raii {
private:
  sd_journal *raw;//My kingdom for a unique_ptr
public:
  sd_journal_raii()
  {
    if (sd_journal_open(&raw, 0) < 0)
      throw runtime_error("Can't open default journal");
  }
  sd_journal_raii(string path)
  {
    if (sd_journal_open_directory(&raw, path.c_str(), 0) < 0)
      throw runtime_error("Can't open journal in path: " + path);
  }

  ~sd_journal_raii(){sd_journal_close(raw);}

  sd_journal_raii(const sd_journal_raii&) = delete;
  sd_journal_raii& operator=(const sd_journal_raii&) = delete;

  operator sd_journal*(){return raw;}
};

class sd_journal_wrap {
private:
  sd_journal_raii journal;

public:
  sd_journal_wrap():journal{}{};
  sd_journal_wrap(string path):journal{path}{};
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
