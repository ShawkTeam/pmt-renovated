/*
 * Copyright (C) 2026 Yağız Zengin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file definations.hpp
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Some concept definitions, descriptions, etc. of the library.
 */

#ifndef LIBHELPER_MACROS_HPP
#define LIBHELPER_MACROS_HPP

#include <type_traits>
#include <iterator>
#include <filesystem>
#include <unistd.h>

/// @brief Default file permissions.
inline constexpr mode_t DEFAULT_FILE_PERMS = 0664;

/// @brief Extended file permissions (using by executables etc).
inline constexpr mode_t DEFAULT_EXTENDED_FILE_PERMS = 0755;

/// @brief Default directory permissions.
inline constexpr mode_t DEFAULT_DIR_PERMS = 0755;

/// @brief Alternative of @c true.
inline constexpr int YES = 1;

/// @brief Alternative of @c false.
inline constexpr int NO = 0;

/// @brief Short names used in dimension type conversions
enum sizeCastTypes {
  B = static_cast<uint8_t>('B'),
  KB = static_cast<uint8_t>('K'),
  MB = static_cast<uint8_t>('M'),
  GB = static_cast<uint8_t>('G')
};

/**
 * @namespace Helper
 * @brief Main namespace of libhelper library.
 */
namespace Helper {

template <typename T, typename = std::void_t<>> struct HasCStrFunction : std::false_type {};

/**
 * @brief Checks whether the input type provides a c_str() member function.
 * @tparam T Input type.
 */
template <typename T> struct HasCStrFunction<T, std::void_t<decltype(std::declval<T>().c_str())>> : std::true_type {};

template <typename Class, typename Type, typename = std::void_t<>> struct HasCloseFunction : std::false_type {};

/**
 * @brief Checks whether the input type provides a valid close() member function.
 * @tparam Class Input type.
 * @tparam Type @c close() parameter type.
 */
template <typename Class, typename Type>
struct HasCloseFunction<Class, Type, std::void_t<decltype(std::declval<Class &>().close(std::declval<Type &>()))>> : std::true_type {};

/**
 * @brief Shortcut of @c HasCloseFunction<Class, Type>::value.
 * @relates HasCloseFunction
 * @tparam Class Input type.
 * @tparam Type @c close() parameter type.
 */
template <typename Class, typename Type> inline constexpr bool HasCloseFunction_v = HasCloseFunction<Class, Type>::value;

template <typename Class, typename Type, typename = std::void_t<>> struct HasCallOperator : std::false_type {};

/**
 * @brief Checks whether the input type provides a valid operator() member function.
 * @tparam Class Input type.
 * @tparam Type @c operator() parameter type.
 */
template <typename Class, typename Type>
struct HasCallOperator<Class, Type, std::void_t<decltype(std::declval<Class &>().operator()(std::declval<Type &>()))>>
    : std::true_type {};

/**
 * @brief Shortcut of @c HasCallOperator<Class, Type>::value.
 * @relates HasCallOperator
 * @tparam Class Input type.
 * @tparam Type @c operator() parameter type.
 */
template <typename Class, typename Type> inline constexpr bool HasCallOperator_v = HasCallOperator<Class, Type>::value;

/**
 * @brief Shortcut of @c HasCStrFunction<T>::value.
 * @relates HasCStrFunction
 * @tparam T Input type.
 */
template <typename T> inline constexpr bool HasCStrFunction_v = HasCStrFunction<T>::value;

template <typename T, typename = std::void_t<>> struct HasIterableBounds : std::false_type {};

/**
 * @brief Checks whether the input type provides @c begin() and @c end() member functions.
 * @tparam T Input type.
 */
template <typename T>
struct HasIterableBounds<T, std::void_t<decltype(std::begin(std::declval<T &>())), decltype(std::end(std::declval<T &>()))>>
    : std::true_type {};

/**
 * @brief Shortcut of @c HasIterableBounds<T>::value.
 * @relates HasIterableBounds
 * @tparam T Input type.
 */
template <typename T> inline constexpr bool HasIterableBounds_v = HasIterableBounds<T>::value;

/**
 * @brief Checks whether the input is invocable and returns requested type.
 *
 * @tparam Func Input function (specify with using @c decltype() ).
 * @tparam Ret  Needed return type.
 * @tparam Args Arguments of function.
 */
template <typename Func, typename Ret, typename... Args>
inline constexpr bool Invocable_v = std::is_invocable_r_v<Ret, Func &, Args...>;

/**
 * @brief Checks whether the input is a container.
 * @tparam T Input class.
 */
template <typename T> struct IsContainer {
  using Clean_T = std::remove_cv_t<std::remove_reference_t<T>>;

  static constexpr bool value =
      HasIterableBounds_v<Clean_T> && !std::is_same_v<Clean_T, std::string> && !std::is_same_v<Clean_T, std::string_view>;
};

/**
 * @brief Shortcut of @c IsContainer<T>::value.
 * @relates IsContainer
 * @tparam T Input type.
 */
template <typename T> inline constexpr bool IsContainer_v = IsContainer<T>::value;

/**
 * @brief Add the @c const qualifier to the input type.
 *
 * @code
 * std::is_same_v<DeepConst<int>::type, const int> // Equals to true
 * std::is_same_v<DeepConst<int*>::type, const int*> // Equals to true
 * @endcode
 *
 * @tparam T Input type.
 * @note Other overloads exist to maintain functionality, but they are not mentioned in the documentation. Please refer to the code.
 */
template <typename T> struct DeepConst {
  using type = std::add_const_t<T>;
};

/** @cond */
template <typename T> struct DeepConst<T *> {
  using type = const T *;
};
/** @endcond */

/**
 * @brief Shortcut of @c DeepConst<T>::type.
 *
 * @code
 * std::is_same_v<DeepConst_t<int>, const int> // Equals to true.
 * @endcode
 *
 * @relates DeepConst
 */
template <typename T> using DeepConst_t = typename DeepConst<T>::type;

/**
 * @brief Add the @c const qualifier if input type is @c char*.
 *
 * @code
 * std::is_same_v<ConstIfCharPointer<char*>::type, const char*> // Equals to true.
 * std::is_same_v<ConstIfCharPointer<int>::type, const int> // Equals to false.
 * @endcode
 *
 * @tparam T Input type.
 * @note If the type is not @c char*, @c type is directly determined as the input type.
 */
template <typename T> struct ConstIfCharPointer {
  using type = T;
};

/** @cond */
/// @copydoc ConstIfCharPointer
template <> struct ConstIfCharPointer<char *> {
  using type = const char *;
};

/// @copydoc ConstIfCharPointer
template <> struct ConstIfCharPointer<const char *> {
  using type = const char *;
};
/** @endcond */

/**
 * @brief Shortcut of @c ConstIfCharPointer<T>::type.
 *
 * @code
 * std::is_same_v<ConstIfCharPointer_t<char*>, const char*> // Equals to true.
 * std::is_same_v<ConstIfCharPointer_t<int>, const int> // Equals to false.
 * @endcode
 *
 * @relates ConstIfCharPointer
 */
template <typename T> using ConstIfCharPointer_t = typename ConstIfCharPointer<T>::type;

/**
 * @brief Verify that the type is @c std::string or @c std::filesystem::path.
 * @tparam T Type.
 * @note References are not accepted.
 */
template <typename T>
inline constexpr bool IsStringOrPath_v =
    !std::is_reference_v<T> && (std::is_same_v<T, std::filesystem::path> || std::is_same_v<T, std::string>);

/**
 * @brief Verify that the type is a size type (unsigned, integral or floating point).
 * @tparam T Type.
 * @note References are not accepted.
 */
template <typename T>
inline constexpr bool IsSizeType_v =
    !std::is_reference_v<T> && std::is_unsigned_v<T> && (std::is_integral_v<T> || std::is_floating_point_v<T>);

template <typename T, typename = std::void_t<>> struct IsEqualityComparable : std::false_type {};

/**
 * @brief Checks whether the input type is equality comparable.
 * @tparam T Input type.
 */
template <typename T>
struct IsEqualityComparable<
    T, std::void_t<decltype(std::declval<T &>() == std::declval<T &>()), decltype(std::declval<T &>() != std::declval<T &>())>>
    : std::true_type {};

/**
 * @brief Shortcut of @c IsEqualityComparable<T>::value.
 * @relates IsEqualityComparable
 * @tparam T Input type.
 */
template <typename T> inline constexpr bool IsEqualityComparable_v = IsEqualityComparable<T>::value;

template <typename T, typename = std::void_t<>> struct HasPathLikeSyntax : std::false_type {};

/**
 * @brief Checks whether the input type has path like syntax.
 * @tparam T Type
 */
template <typename T>
struct HasPathLikeSyntax<
    T,
    std::void_t<decltype(std::declval<T &>().append(std::declval<std::string>())),
                decltype(std::declval<T &>().append(std::declval<const char *>())),

                std::enable_if_t<IsStringOrPath_v<decltype(std::declval<T &>().filename())>>,

                std::enable_if_t<std::is_same_v<std::decay_t<T>, std::filesystem::path> ||
                                 std::is_convertible_v<decltype(std::declval<T &>().string()), std::string>>,

                decltype(std::declval<T &>() = std::declval<const T &>()), decltype(std::declval<T &>() == std::declval<const T &>()),
                decltype(std::declval<T &>() != std::declval<const T &>()), decltype(std::declval<T &>() = std::declval<T &&>())>>
    : std::true_type {};

/// @brief Verify that the type has similar properties to @c std::filesystem::path.
template <typename T>
struct IsPathTypeLike : std::conjunction<HasPathLikeSyntax<T>, std::is_default_constructible<T>, std::is_constructible<T, std::string>,
                                         std::is_constructible<T, const char *>, std::is_nothrow_move_constructible<T>,
                                         std::is_copy_assignable<T>, std::is_move_assignable<T>> {};

/**
 * @brief Shortcut of @c IsPathTypeLike<T>::value.
 * @relates IsPathTypeLike
 * @tparam T Input type.
 */
template <typename T> inline constexpr bool IsPathTypeLike_v = IsPathTypeLike<T>::value;

template <typename T, typename Ret, typename = std::void_t<>> struct HasFirstMember : std::false_type {};

/**
 * @brief Checks whether the input type has a @c first member.
 *
 * @tparam T Input type.
 * @tparam Ret Needed type of @c first member.
 */
template <typename T, typename Ret>
struct HasFirstMember<T, Ret,
                      std::void_t<decltype(std::declval<T &>().first),
                                  std::enable_if_t<std::is_convertible_v<decltype(std::declval<T &>().first), Ret>>>>
    : std::true_type {};

/**
 * @brief Shortcut of @c HasFirstMember<T, Ret>::value.
 * @relates HasFirstMember
 * @tparam T Input type.
 * @tparam Ret Needed type of @c first member.
 */
template <typename T, typename Ret> inline constexpr bool HasFirstMember_v = HasFirstMember<T, Ret>::value;

template <typename T, typename Ret, typename = std::void_t<>> struct HasSecondMember : std::false_type {};

/**
 * @brief Checks whether the input type has a @c second member.
 *
 * @tparam T Input type.
 * @tparam Ret Needed type of @c second member.
 */
template <typename T, typename Ret>
struct HasSecondMember<T, Ret,
                       std::void_t<decltype(std::declval<T &>().second),
                                   std::enable_if_t<std::is_convertible_v<decltype(std::declval<T &>().second), Ret>>>>
    : std::true_type {};

/**
 * @brief Shortcut of @c HasSecondMember<T, Ret>::value.
 * @relates HasSecondMember
 * @tparam T Input type.
 * @tparam Ret Needed type of @c second member.
 */
template <typename T, typename Ret> inline constexpr bool HasSecondMember_v = HasSecondMember<T, Ret>::value;

/// @brief Always false.
template <typename = std::void_t<>> struct AlwaysFalse : std::false_type {};

/// @brief Shortcut of @c AlwaysFalse<T>::value.
template <typename T = std::void_t<>> inline constexpr bool AlwaysFalse_v = AlwaysFalse<T>::value;

/// @brief Always true.
template <typename = std::void_t<>> struct AlwaysTrue : std::true_type {};

/// @brief Shortcut of @c AlwaysTrue<T>::value.
template <typename T = std::void_t<>> inline constexpr bool AlwaysTrue_v = AlwaysTrue<T>::value;

/**
 * @brief Source location (it is similar to std::source_location in C++20).
 * @note This is a compile-time only structure.
 */
struct SourceLocation {
private:
  static constexpr const char *strip_path(const char *path) noexcept {
    const char *file = path;
    while (*path) {
      if (*path == '/' || *path == '\\') file = path + 1;
      path++;
    }
    return file;
  }

public:
  const char *file_path;
  const char *file_name;
  const char *function;
  int line;

  static constexpr SourceLocation current(
#if defined(__GNUC__) || defined(__clang__)
      const char *file_ = __builtin_FILE(), const char *func_ = __builtin_FUNCTION(), int line_ = __builtin_LINE()
#else
      const char *file_ = "unknown", const char *func_ = "unknown", const char *sig_ = "unknown", int line = 0
#endif
          ) noexcept {
    SourceLocation loc{};
    loc.file_path = file_;
    loc.file_name = strip_path(file_);
    loc.function = func_;
    loc.line = line_;
    return loc;
  }
};

} // namespace Helper

/// @brief Name shortcut for logging.
#define HELPER "libhelper"

/**
 * @brief Take @c x byte as bits.
 *
 * @code
 * uint64_t size = B(4); // 32, 4 * 8
 * @endcode
 *
 * @param x Size in bytes.
 */
#define B(x) (static_cast<uint64_t(x)> * 8)

/**
 * @brief Take @c x kilobyte as bytes.
 *
 * @code
 * uint64_t size = KB(8); // 8192, 8 * 1024
 * @endcode
 *
 * @param x Size in kilobytes.
 */
#define KB(x) (static_cast<uint64_t>(x) * 1024)

/**
 * @brief Take @c x megabyte as bytes.
 *
 * @code
 * uint64_t size = MB(4); // 4194304, KB(4) * 1024
 * @endcode
 *
 * @param x Size in megabytes.
 */
#define MB(x) (KB(x) * 1024)

/**
 * @brief Take @c x gigabyte as bytes.
 *
 * @code
 * uint64_t size = GB(1); // 1073741824, MB(1) * 1024
 * @endcode
 *
 * @param x Size in gigabytes.
 */
#define GB(x) (MB(x) * 1024)

/**
 * @brief Convert @c x byte to kilobyte.
 *
 * @code
 * uint32_t size = TO_KB(1024); // 1, 1024 / 1024
 * @endcode
 *
 * @param x Size in bytes.
 */
#define TO_KB(x) (x / 1024)

/**
 * @brief Convert @c x byte to megabyte.
 *
 * @code
 * uint32_t size = TO_KB(2048); // 2, TO_KB(2048) / 1024
 * @endcode
 *
 * @param x Size in bytes.
 */
#define TO_MB(x) (TO_KB(x) / 1024)

/**
 * @brief Convert @c x byte to gigabyte.
 *
 * @code
 * uint32_t size = TO_GB(1048576); // 1, TO_MB(1048576) / 1024
 * @endcode
 *
 * @param x Size in bytes.
 */
#define TO_GB(x) (TO_MB(x) / 1024)

/**
 * @name Terminal ANSI Style Codes
 * @brief Macros for text formatting in ANSI-compatible terminals.
 * @{
 */
#define STYLE_RESET "\033[0m"     ///< Resets all formatting to default.
#define BOLD "\033[1m"            ///< Makes the text bold or high intensity.
#define FAINT "\033[2m"           ///< Makes the text faint (dimmed).
#define ITALIC "\033[3m"          ///< Italicizes the text (not supported by all terminals).
#define UNDERLINE "\033[4m"       ///< Draws a line under the text.
#define BLINC "\033[5m"           ///< Slow blink (less than 150 per minute).
#define FAST_BLINC "\033[6m"      ///< Rapid blink (not widely supported).
#define STRIKE_THROUGHT "\033[9m" ///< Draws a line through the text.
#define NO_UNDERLINE "\033[24m"   ///< Disables underlining.
#define NO_BLINC "\033[25m"       ///< Disables blinking.
/** @} */

/**
 * @name Terminal ANSI Color Codes
 * @brief Macros for foreground (text) and background coloring.
 * @{
 */

// Foreground (text) colors
#define BLACK "\033[30m"   ///< Black text.
#define RED "\033[31m"     ///< Red text.
#define GREEN "\033[32m"   ///< Green text.
#define YELLOW "\033[33m"  ///< Yellow text.
#define BLUE "\033[34m"    ///< Blue text.
#define MAGENTA "\033[35m" ///< Magenta (Purple) text.
#define CYAN "\033[36m"    ///< Cyan (Light Blue) text.
#define WHITE "\033[37m"   ///< White text.

// Bright foreground colors
#define BRIGHT_BLACK "\033[90m"   ///< Bright Black (Gray) text.
#define BRIGHT_RED "\033[91m"     ///< Bright Red text.
#define BRIGHT_GREEN "\033[92m"   ///< Bright Green text.
#define BRIGHT_YELLOW "\033[93m"  ///< Bright Yellow text.
#define BRIGHT_BLUE "\033[94m"    ///< Bright Blue text.
#define BRIGHT_MAGENTA "\033[95m" ///< Bright Magenta text.
#define BRIGHT_CYAN "\033[96m"    ///< Bright Cyan text.
#define BRIGHT_WHITE "\033[97m"   ///< Bright White text.

// Background colors
#define BG_BLACK "\033[40m"   ///< Black background.
#define BG_RED "\033[41m"     ///< Red background.
#define BG_GREEN "\033[42m"   ///< Green background.
#define BG_YELLOW "\033[43m"  ///< Yellow background.
#define BG_BLUE "\033[44m"    ///< Blue background.
#define BG_MAGENTA "\033[45m" ///< Magenta background.
#define BG_CYAN "\033[46m"    ///< Cyan background.
#define BG_WHITE "\033[47m"   ///< White background.
/** @} */

#ifndef HELPER_NO_C_TYPE_HANDLERS
// ABORT(message), ex: ABORT("memory error!\n")
#define ABORT(msg)                                                                                                                    \
  do {                                                                                                                                \
    fprintf(stderr, "%s%sCRITICAL ERROR%s: %s\nAborting...\n", BOLD, RED, STYLE_RESET, msg);                                          \
    abort();                                                                                                                          \
  } while (0)

// ERROR(message, exit), ex: ERROR("an error occurred.\n", 1)
#define ERROR(msg, code)                                                                                                              \
  do {                                                                                                                                \
    fprintf(stderr, "%s%sERROR%s: %s", BOLD, RED, STYLE_RESET, msg);                                                                  \
    exit(code);                                                                                                                       \
  } while (0)

// WARNING(message), ex: WARNING("using default setting.\n")
#define WARNING(msg) fprintf(stderr, "%s%sWARNING%s: %s", BOLD, YELLOW, STYLE_RESET, msg);

// INFO(message), ex: INFO("operation ended.\n")
#define INFO(msg) fprintf(stdout, "%s%sINFO%s: %s", BOLD, GREEN, STYLE_RESET, msg);
#endif // #ifndef HELPER_NO_C_TYPE_HANDLERS

/**
 * @brief Generate version string and return.
 *
 * @code
 * std::string getLibVersion(void) { MKVERSION("libhelper"); }
 * @endcode
 *
 * @param name Target name.
 * @note This is a complete function body!!!
 */
#define MKVERSION(name)                                                                                                               \
  char vinfo[512];                                                                                                                    \
  sprintf(vinfo, "%s %s %s-%s-%s [API %d, %s %s]\n%s", name, BUILD_ABI, BUILD_VERSION, COMMIT_ID, BUILD_TYPE, __ANDROID_API__,        \
          BUILD_DATE, BUILD_TIME, BUILD_COMPILER_VERSION);                                                                            \
  return std::string(vinfo)

#endif // #ifndef LIBHELPER_MACROS_HPP
