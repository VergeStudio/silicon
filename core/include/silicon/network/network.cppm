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
// 本模块无 :config 分区：版本信息统一由 silicon.core::get_version* 提供。

export module silicon.network;
export import silicon.network.error;

export import :facade;
export import :dns;
export import :tcp;
export import :udp;
#ifdef SILICON_FEATURE_TLS
export import :tls;
#endif
