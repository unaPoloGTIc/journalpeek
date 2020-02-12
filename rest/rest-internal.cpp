#include "rest-internal.h"

string messageLiteral = "MESSAGE"s;

handlersMap jdwrapper{
      {"/v1/paged_search",
	  [](web::http::http_request req, web::json::value jvals) {
	    auto rep = web::json::value::object();
	    sd_journal_wrap journal{"./testdata/"s};
	    int offset{jvals.has_integer_field("offset")?jvals["offset"].as_integer():0};
	    int size{jvals.has_integer_field("size")?jvals["size"].as_integer():0};
	    /* TODO:
	    web::json::array exact{jvals.has_array_field("matches")?
				   jvals["matches"].as_array():
				   web::json::value::array().as_array()};
	    */
	    if (jvals.has_string_field("match"))
	      {
		auto tmp{jvals["match"].as_string()};
		auto loc{tmp.find('=')};
		journal.addExactMessageMatch(tmp.substr(0,loc),tmp.substr(loc+1, tmp.npos));
	      }
	    string reg{jvals.has_string_field("regex")?jvals["regex"].as_string():""s};
	    bool ignoreCase{jvals.has_boolean_field("")?jvals["ignore_case"].as_bool():false};
	    auto allMsgs{journal.vec_msgs(reg, ignoreCase)};//TODO: don't recreate for each request
	    vector<web::json::value> allVals;
	    for(const auto& m:allMsgs)///TODO: use offset, size
		auto r = allVals.emplace_back(web::json::value::string(m));
	    rep["resp"] = web::json::value::array(allVals);
	    req.reply(web::http::status_codes::OK, rep).wait();
	    return;
	  }},
};

restServer::restServer(handlersMap endpoints, string port)
    : s("http://0.0.0.0:"s + port + "/") {
  s.support(web::http::methods::GET, [endpoints](web::http::http_request req) {
    auto u{req.relative_uri().to_string()};
    try {
      auto jvals{req.extract_json().get()};
      auto endpoint{req.relative_uri().to_string()};
      endpoints.at(endpoint)(req, jvals);
    } catch (...) {
      req.reply(web::http::status_codes::NotFound, "Error handling request")
          .wait();
    }
  });

  auto waiter{s.open()};
  if (waiter.wait() != pplx::completed)
    throw runtime_error("can't start listener");
}

restServer::~restServer() { s.close().wait(); }
