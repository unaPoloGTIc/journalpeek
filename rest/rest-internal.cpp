#include "rest-internal.h"

string messageLiteral = "MESSAGE"s;

restServer::restServer(handlersMap endpoints,
                       string port = "6666"s) // TODO: https
    : s("http://0.0.0.0:"s + port + "/") {
  s.support(web::http::methods::GET, [endpoints](web::http::http_request req) {
    auto u{req.relative_uri().to_string()};
    try {
      auto jvals{req.extract_json().get()};
      if (!verifyToken("TODO")) {
        auto rep = web::json::value::object();
        rep["error"] = web::json::value::string("Bad token"s);
        req.reply(web::http::status_codes::OK, rep).wait();
        return;
      }
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

handlersMap jdwrapper{
    {"/v1/example",

     [](web::http::http_request req, web::json::value jvals) {
       auto rep = web::json::value::object();
       string val1{jvals["val1"].as_string()};
       // do something with val1
       rep["resp1"] = web::json::value::string("resp1"s);
       req.reply(web::http::status_codes::OK, rep).wait();
       return;
     }},
};
