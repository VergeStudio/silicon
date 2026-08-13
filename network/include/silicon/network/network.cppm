// Primary module interface for silicon::network.
//
// silicon::network is a genuine C++20 named-module library (previously a
// classic header + TU library). The public API is split across interface
// partitions (:core, :dns, :tcp, :udp, :tls) so that class *declarations*
// live in the module purview (not the global module fragment). This lets the
// implementation units (module silicon.network;) provide out-of-line member
// definitions without triggering the classic "declaration follows declaration
// in the global module" error.
//
// The :config partition is generated from network.config.cppm.in by xmake and
// is added from $(builddir)/silicon/config.

export module silicon.network;
export import silicon.network.error;

export import :config;
export import :core;
export import :dns;
export import :tcp;
export import :udp;
#ifdef SILICON_FEATURE_TLS
export import :tls;
#endif
