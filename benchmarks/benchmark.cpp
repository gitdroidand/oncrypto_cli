#include "oncrypto/oncrypto.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

namespace {

struct Sample {
    std::string label;
    double elapsed_ms = 0.0;
    double throughput_mb_s = 0.0;
};

std::vector<std::size_t> sizes() {
    return {
        1024,
        4 * 1024,
        16 * 1024,
        64 * 1024,
        256 * 1024,
        1024 * 1024,
        4 * 1024 * 1024,
        16 * 1024 * 1024,
        64 * 1024 * 1024
    };
}

std::vector<unsigned char> make_payload(std::size_t n) {
    std::vector<unsigned char> payload(n);
    for (std::size_t i = 0; i < n; ++i) {
        payload[i] = static_cast<unsigned char>((i * 37u) % 251u);
    }
    return payload;
}

template <typename Fn>
Sample time_case(const std::string& label, std::size_t bytes, Fn&& fn) {
    auto payload = make_payload(bytes);
    auto start = std::chrono::steady_clock::now();
    auto result = fn(payload);
    auto end = std::chrono::steady_clock::now();
    const double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    const double throughput = (result.size() > 0 && elapsed_ms > 0.0)
        ? (static_cast<double>(bytes) / (1024.0 * 1024.0) / (elapsed_ms / 1000.0))
        : 0.0;
    (void)result;
    return {label, elapsed_ms, throughput};
}

} // namespace

int main() {
    std::cout << "OnCrypto benchmark scaffold\n";
    std::cout << "Sizes (bytes):";
    for (auto size : sizes()) {
        std::cout << ' ' << size;
    }
    std::cout << "\n\n";

    for (auto size : sizes()) {
        std::string password = "bench-password";
        std::vector<unsigned char> payload = make_payload(size);

        auto t1 = std::chrono::steady_clock::now();
        auto encrypted = onc::encrypt(payload, password);
        auto t2 = std::chrono::steady_clock::now();
        auto t3 = std::chrono::steady_clock::now();
        auto decrypted = onc::decrypt(encrypted, password);
        auto t4 = std::chrono::steady_clock::now();

        const double enc_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        const double dec_ms = std::chrono::duration<double, std::milli>(t4 - t3).count();
        const double enc_mb_s = (enc_ms > 0.0) ? (static_cast<double>(size) / (1024.0 * 1024.0)) / (enc_ms / 1000.0) : 0.0;
        const double dec_mb_s = (dec_ms > 0.0) ? (static_cast<double>(size) / (1024.0 * 1024.0)) / (dec_ms / 1000.0) : 0.0;

        std::cout << "size=" << size
                  << " enc_ms=" << std::fixed << std::setprecision(3) << enc_ms
                  << " enc_mb_s=" << enc_mb_s
                  << " dec_ms=" << dec_ms
                  << " dec_mb_s=" << dec_mb_s
                  << "\n";
    }

    return 0;
}
