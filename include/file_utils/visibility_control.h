#ifndef FILE_UTILS__VISIBILITY_CONTROL_H_
#define FILE_UTILS__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define FILE_UTILS_EXPORT __attribute__((dllexport))
#define FILE_UTILS_IMPORT __attribute__((dllimport))
#else
#define FILE_UTILS_EXPORT __declspec(dllexport)
#define FILE_UTILS_IMPORT __declspec(dllimport)
#endif
#ifdef FILE_UTILS_BUILDING_LIBRARY
#define FILE_UTILS_PUBLIC FILE_UTILS_EXPORT
#else
#define FILE_UTILS_PUBLIC FILE_UTILS_IMPORT
#endif
#define FILE_UTILS_PUBLIC_TYPE FILE_UTILS_PUBLIC
#define FILE_UTILS_LOCAL
#else
#define FILE_UTILS_EXPORT __attribute__((visibility("default")))
#define FILE_UTILS_IMPORT
#if __GNUC__ >= 4
#define FILE_UTILS_PUBLIC __attribute__((visibility("default")))
#define FILE_UTILS_LOCAL __attribute__((visibility("hidden")))
#else
#define FILE_UTILS_PUBLIC
#define FILE_UTILS_LOCAL
#endif
#define FILE_UTILS_PUBLIC_TYPE
#endif

#endif  // FILE_UTILS__VISIBILITY_CONTROL_H_
