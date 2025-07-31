#ifndef WRITELOG_HPP
#define WRITELOG_HPP

//// 检查是否支持C++14
//#if _MSVC_LANG < 201402L && __cplusplus < 201402L
//#error "此文件需要编译器支持 C++14 标准"
//#endif

// 检查是否支持C++17
#if _MSVC_LANG < 201703L && __cplusplus < 201703L
#error "此文件需要编译器支持 C++17 标准"
#endif

namespace wlhsb {

extern "C" void TKWriteLog(const char* format, ...);

} // namespace wlhsb


#endif // WRITELOG_HPP
