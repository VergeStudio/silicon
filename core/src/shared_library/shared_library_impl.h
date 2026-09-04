#pragma once













namespace silicon::library {

struct shared_library::impl {
    std::string path_;
    void *handle_{nullptr};
    std::mutex mutex_;
};

}
