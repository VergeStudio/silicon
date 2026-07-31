module;
#include <memory>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

module silicon.http;

namespace silicon::http {

FakeHttpClient::FakeHttpClient(HttpResponse response)
    : m_p(std::make_unique<P>()) { m_p->response_ = std::move(response); }

HttpResponse FakeHttpClient::request(const HttpRequest &) const {
    ++m_p->call_count_;
    return m_p->response_;
}

std::size_t FakeHttpClient::call_count() const {
    return m_p->call_count_;
}

HttpResponse CurlHttpClient::request(const HttpRequest &req) const {
    // 构建 curl 命令（简略版，仅支持 GET/POST）
    std::string cmd = "curl -s -w '\\n%{http_code}' -m " + std::to_string(req.timeout_ms() / 1000);
    if(req.method() == "POST") {
        cmd += " -X POST";
        if(!req.body().empty()) {
            cmd += " -d '" + req.body() + "'";
        }
    }
    cmd += " '" + req.url() + "' 2>/dev/null";

#ifdef _WIN32
    FILE *pipe = _popen(cmd.c_str(), "r");
#else
    FILE *pipe = popen(cmd.c_str(), "r");
#endif
    HttpResponse resp;
    if(!pipe) {
        resp.status_code() = 0;
        return resp;
    }

    std::string all;
    char buf[4096];
    while(fgets(buf, sizeof(buf), pipe)) all += buf;
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    // 最后一行是 status code
    auto nl_pos = all.rfind('\n');
    if(nl_pos != std::string::npos && nl_pos > 0) {
        resp.body() = all.substr(0, --nl_pos);
        auto sc_str = all.substr(nl_pos + 1);
        resp.status_code() = std::atoi(sc_str.c_str());
    }
    return resp;
}

HttpResponse IHttpClient::get(const std::string &url) const {
    HttpRequest req;
    req.url() = url;
    return request(req);
}

} // namespace silicon::http
