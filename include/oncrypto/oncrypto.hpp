#pragma once

/**
 * OnCrypto - Cross-platform encryption library
 * Version 1.3.0
 * 
 * Usage:
 *   #include <oncrypto/oncrypto.hpp>
 *   auto encrypted = crypto::encrypt(data, password);
 */

#include <vector>
#include <string>

namespace crypto {

// ============================================================
// API عمومی
// ============================================================

/**
 * رمزگذاری داده‌ها با پسوورد
 * @param data داده‌های خام
 * @param password پسوورد
 * @return داده‌های رمز شده (شامل salt + IV + tag + ciphertext)
 */
std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

/**
 * رمزگشایی داده‌ها با پسوورد
 * @param data داده‌های رمز شده
 * @param password پسوورد
 * @return داده‌های اصلی
 */
std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

/**
 * دریافت اطلاعات الگوریتم استفاده شده
 */
std::string getAlgorithmName();

/**
 * دریافت نسخه کتابخانه
 */
std::string getVersion();

} // namespace crypto