#include "rest-internal.h"

string messageLiteral = "MESSAGE"s;

function<void(web::http::http_request &, web::http::http_response &)> corsify =
    [](web::http::http_request &req, web::http::http_response &response) {
      auto headers{req.headers()};
      // web::http::http_response response(web::http::status_codes::NoContent);
      auto sethead = [&headers, &response](string from, string to) {
        auto orig{headers.find(from)};
        response.headers().add(to, orig == headers.end() ? "*" : orig->second);
      };
      // response.headers().add("Access-Control-Allow-Credentials", "true");
      sethead("Access-Control-Request-Headers", "Access-Control-Allow-Headers");
      sethead("Access-Control-Request-Method", "Access-Control-Allow-Methods");
      sethead("Origin", "Access-Control-Allow-Origin");
      req.reply(response).wait();
    };

handlersMap jdwrapper{
    {"/v0/paged_search",
     [](web::http::http_request req, web::json::value jvals) {
       auto rep = web::json::value::object();
       sd_journal_wrap journal{"./testdata/"s};
       // int
       // offset{jvals.has_integer_field("offset")?jvals["offset"].as_integer():0};
       // int
       // size{jvals.has_integer_field("size")?jvals["size"].as_integer():0};
       /* TODO:
       web::json::array exact{jvals.has_array_field("matches")?
                              jvals["matches"].as_array():
                              web::json::value::array().as_array()};
       */
       if (jvals.has_string_field("match")) {
         auto tmp{jvals["match"].as_string()};
         auto loc{tmp.find('=')};
         journal.addExactMessageMatch(tmp.substr(0, loc),
                                      tmp.substr(loc + 1, tmp.npos));
       }
       string reg{jvals.has_string_field("regex") ? jvals["regex"].as_string()
                                                  : ""s};
       bool ignoreCase{jvals.has_boolean_field("")
                           ? jvals["ignore_case"].as_bool()
                           : false};
       auto allMsgs{journal.vec_msgs(
           reg, ignoreCase)}; // TODO: don't recreate for each request
       vector<web::json::value> allVals;
       for (const auto &m : allMsgs) /// TODO: use offset, size
         auto r = allVals.emplace_back(web::json::value::string(m));

       rep["resp"] = web::json::value::array(allVals);
       web::http::http_response response(web::http::status_codes::OK);
       response.set_body(rep);
       corsify(req, response);
       return;
     }},
    {"/v1/paged_search",
     [](web::http::http_request req, web::json::value jvals) {
       auto rep = web::json::value::object();
       sd_journal_wrap journal{
           "./testdata/"s}; // sd_journal precludes multiple threads
       string begin{jvals.has_string_field("begin") ? jvals["begin"].as_string()
                                                    : ""s};
       int pagesize{jvals.has_integer_field("pagesize")
                        ? jvals["pagesize"].as_integer()
                        : 100};

       if (jvals.has_string_field("match")) {
         auto tmp{jvals["match"].as_string()};
         if (auto loc{tmp.find('=')}; loc != tmp.npos)
           journal.addExactMessageMatch(tmp.substr(loc + 1, tmp.npos),
                                        tmp.substr(0, loc));
         else
           journal.addExactMessageMatch(tmp);
       }

       string regex{jvals.has_string_field("regex") ? jvals["regex"].as_string()
                                                    : ""s};
       bool ignoreCase{jvals.has_boolean_field("ignore_case")
                           ? jvals["ignore_case"].as_bool()
                           : false};
       bool backwards{jvals.has_boolean_field("backwards")
                          ? jvals["backwards"].as_bool()
                          : false};
       auto [vec, end, eof]{
           journal.paged_msgs(begin, pagesize, regex, ignoreCase, backwards)};

       vector<web::json::value> allVals;
       for (const auto &m : vec)
         auto r = allVals.emplace_back(web::json::value::string(m));

       rep["items"] = web::json::value::array(allVals);
       rep["end"] = web::json::value::string(end);
       rep["eof"] = web::json::value::boolean(eof);

       web::http::http_response response(web::http::status_codes::OK);
       response.set_body(rep);
       corsify(req, response);
       return;
     }},

    {"/v1/all_fields",
     [](web::http::http_request req, web::json::value) {
       sd_journal_wrap journal{
           "./testdata/"s}; // sd_journal precludes multiple threads
       auto vec{journal.fieldnames()};

       vector<web::json::value> allVals;
       for (const auto &m : vec)
         auto r = allVals.emplace_back(web::json::value::string(m));

       auto rep{web::json::value::array(allVals)};

       web::http::http_response response(web::http::status_codes::OK);
       response.set_body(rep);
       corsify(req, response);
       return;
     }},

    {"/v1/field_unique",
     [](web::http::http_request req, web::json::value jvals) {
       string field{jvals.has_string_field("field") ? jvals["field"].as_string()
                                                    : ""s};
       sd_journal_wrap journal{
           "./testdata/"s}; // sd_journal precludes multiple threads

       auto vec{journal.fieldUnique(field)};

       vector<web::json::value> allVals;
       for (const auto &m : vec)
         auto r = allVals.emplace_back(web::json::value::string(m));

       auto rep{web::json::value::array(allVals)};
       web::http::http_response response(web::http::status_codes::OK);
       response.set_body(rep);
       corsify(req, response);
       return;
     }},

};

//TODO: solve unsafe-port issues (chrome, firefox)
restServer::restServer(handlersMap endpoints, string proto)
  : cw{}, s(proto+"://0.0.0.0:6666/"s, cw.get()) {
  auto responder = [endpoints](web::http::http_request req) {
    auto u{req.relative_uri().to_string()};
    try {
      auto jvals{req.extract_json().get()};
      auto endpoint{req.relative_uri().to_string()};
      endpoints.at(endpoint)(req, jvals);
    } catch (...) {

      auto rep{web::json::value::string("Error handling request"s)};
      web::http::http_response response(web::http::status_codes::NotFound);
      response.set_body(rep);
      corsify(req, response);
    }
  };
  s.support(web::http::methods::GET, responder);
  /*F Angular*/
  s.support(web::http::methods::POST, responder);
  /*F CORS*/
  s.support(
      web::http::methods::OPTIONS, [endpoints](web::http::http_request req) {
        web::http::http_response response(web::http::status_codes::NoContent);
        corsify(req, response);
      });

  auto waiter{s.open()};
  if (waiter.wait() != pplx::completed)
    throw runtime_error("can't start listener");
}

restServer::~restServer() { s.close().wait(); }
