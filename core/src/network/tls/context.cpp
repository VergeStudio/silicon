

module;

#ifdef SILICON_FEATURE_TLS
#    include <openssl/err.h>
#    include <openssl/ssl.h>

#    include <expected>
#    include <filesystem>
#    include <mutex>
#    include <system_error>
#    include <utility>
#endif

module silicon.network;

#ifdef SILICON_FEATURE_TLS

namespace silicon::network::tls {
static uint64_t g_tls_context_count{0};
static std::mutex g_tls_context_mutex{};

auto context::create(verify_peer_t verify_peer) -> network::result<context> {
    {
        std::scoped_lock g{g_tls_context_mutex};
        if(g_tls_context_count == 0) {
#    if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x10100000L
            OPENSSL_init_ssl(0, nullptr);
#    else
            SSL_library_init();
#    endif
        }
        ++g_tls_context_count;
    }

#    if !defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x10100000L
    auto *ssl_ctx = SSL_CTX_new(TLS_method());
#    else
    auto *ssl_ctx = SSL_CTX_new(SSLv23_method());
#    endif
    if(ssl_ctx == nullptr) {
        return std::unexpected(make_error_code(network_error::kTlsContextInitFailed));
    }


    context ctx{ssl_ctx};


    SSL_CTX_set_options(ssl_ctx, SSL_OP_ALL | SSL_OP_NO_SSLv3);

    if(verify_peer == verify_peer_t::kYes) {
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, NULL);
    }

    SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);

    return ctx;
}

auto context::create(
        std::filesystem::path certificate,
        tls_file_type certificate_type,
        std::filesystem::path private_key,
        tls_file_type private_key_type,
        verify_peer_t verify_peer
) -> network::result<context> {
    auto ctx = create(verify_peer);
    if(!ctx) {
        return ctx;
    }

    auto *ssl_ctx = ctx->m_ssl_ctx;

    if(auto r = SSL_CTX_use_certificate_file(ssl_ctx, certificate.c_str(), static_cast<int>(certificate_type));
       r != 1) {
        return std::unexpected(make_error_code(network_error::kTlsCertificateLoadFailed));
    }

    if(auto r = SSL_CTX_use_PrivateKey_file(ssl_ctx, private_key.c_str(), static_cast<int>(private_key_type));
       r != 1) {
        return std::unexpected(make_error_code(network_error::kTlsPrivateKeyLoadFailed));
    }

    if(auto r = SSL_CTX_check_private_key(ssl_ctx); r != 1) {
        return std::unexpected(make_error_code(network_error::kTlsKeyMismatch));
    }

    return ctx;
}

context::~context() {
    if(m_ssl_ctx != nullptr) {
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = nullptr;
    }
}

}

#endif
