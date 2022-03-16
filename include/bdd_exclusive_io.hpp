//
// Created by byjtew on 14/03/2022.
//

#ifndef C_BDD_BDD_EXCLUSIVE_IO_HPP
#define C_BDD_BDD_EXCLUSIVE_IO_HPP

#pragma GCC diagnostic ignored "-Wformat-security"

#define EXCLUSIVE_IO_MMAP_NAME "/tmp/mmap_bdd_stdoutmutex"

#include <fstream>
#include <pthread.h>
#include <iomanip>
#include <iostream>
#include <execution>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/printf.h>

typedef struct synchronisation_mutex {
    pthread_mutex_t print_mutex;
    pthread_mutex_t log_print_mutex;;
} synchronisation_mutex_t;

class ExclusiveIO {
private:

    // Source: https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_.28Select_Graphic_Rendition.29_parameters
    typedef enum {
        FG_BRIGHT_RED = 91, FG_RED = 31,
        FG_BRIGHT_GREEN = 92, FG_GREEN = 32,
        FG_BRIGHT_YELLOW = 93, FG_YELLOW = 33,
        FG_BRIGHT_BLUE = 94, FG_BLUE = 34,
        FG_BRIGHT_MAGENTA = 95, FG_MAGENTA = 35,
        FG_BRIGHT_CYAN = 96, FG_CYAN = 36,
        FG_BRIGHT_WHITE = 97, FG_WHITE = 37, FG_GRAY = 90,
        FG_DEFAULT = 39,
    } color_t;

    inline static pid_t parent_pid;
    inline static synchronisation_mutex_t *synchronisation_map;
    inline static std::ofstream log_ofstream;


    static void lockLogPrint() {
      pthread_mutex_lock(&synchronisation_map->log_print_mutex);
    }

    static void unlockLogPrint() {
      pthread_mutex_unlock(&synchronisation_map->log_print_mutex);
    }

    static bool isParent() {
      return getpid() == parent_pid;
    }

    static std::ofstream &getLogStream() {
      return log_ofstream;
    }

    template<typename... Args>
    static std::string formatArgs(const std::string_view &rt_fmt_str, Args &&... args) {
      // Using C formatting instead of LibFMT.
      std::string buffer;
      buffer.resize(rt_fmt_str.length() + 132);
      auto size = std::sprintf(buffer.data(), rt_fmt_str.data(), args...);
      buffer.resize(size);
      return buffer;
    }

    template<typename... Args>
    static void
    genericFormatPrint(std::ostream &out, ExclusiveIO::color_t color, const std::string_view &rt_fmt_str,
                       Args &&... args) {
      lockPrint();
      out << "\033[" << color << "m" << (isParent() ? "[BDD]: " : "[Child]: ")
          << formatArgs(rt_fmt_str, args...) << "\033[0m" << std::flush;
      unlockPrint();
    }

    template<typename... Args>
    inline static void
    genericNotFormatPrint(std::ostream &out, ExclusiveIO::color_t color, Args &&... args) {
      lockPrint();
      out << "\033[" << color << "m" << (isParent() ? "[BDD]: " : "[Child]: ");
      ((out << args), ...);
      out << "\033[0m" << std::flush;
      unlockPrint();
    }

    template<typename... Args>
    static void genericFilePrint(const std::string_view &rt_fmt_str, Args &&... args) {
      lockLogPrint();
      getLogStream() << formatArgs(rt_fmt_str, args...);
      unlockLogPrint();
    }

    static void createLogFile();

    static void mmapExclusionStructure();

public:
    static void initialize(pid_t parentPid);

    static void terminate();

    static void lockPrint() {
      pthread_mutex_lock(&synchronisation_map->print_mutex);
    }

    static void unlockPrint() {
      pthread_mutex_unlock(&synchronisation_map->print_mutex);
    }

#pragma region Formatted print

    template<typename... Args>
    static void info_f(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericFormatPrint(std::cout, isParent() ? color_t::FG_BLUE : color_t::FG_GREEN, rt_fmt_str,
                                      std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error_f(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericFormatPrint(std::cerr, color_t::FG_RED, rt_fmt_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void hint_f(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericFormatPrint(std::cout, color_t::FG_CYAN, rt_fmt_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug_f(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericFormatPrint(std::cout, color_t::FG_GRAY, rt_fmt_str, std::forward<Args>(args)...);
      //ExclusiveIO::genericFilePrint(rt_fmt_str, std::forward<>(args)...);
    }

    template<typename... Args>
    static void debugError_f(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericFormatPrint(std::cout, color_t::FG_BRIGHT_WHITE, rt_fmt_str, std::forward<Args>(args)...);
      //ExclusiveIO::genericFilePrint(rt_fmt_str, std::forward<>(args)...);
    }

#pragma endregion

#pragma region Unformatted print

    template<typename... Args>
    static void infoHigh_nf(Args &&... args) {
      ExclusiveIO::genericNotFormatPrint(std::cout, isParent() ? color_t::FG_BRIGHT_BLUE : color_t::FG_BRIGHT_GREEN,
                                         std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info_nf(Args &&... args) {
      ExclusiveIO::genericNotFormatPrint(std::cout, isParent() ? color_t::FG_BLUE : color_t::FG_GREEN,
                                         std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error_nf(Args &&... args) {
      ExclusiveIO::genericNotFormatPrint(std::cerr, color_t::FG_RED, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void hint_nf(Args &&... args) {
      ExclusiveIO::genericNotFormatPrint(std::cout, color_t::FG_CYAN, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug_nf(Args &&... args) {
      ExclusiveIO::genericNotFormatPrint(std::cout, color_t::FG_GRAY, std::forward<Args>(args)...);
      //ExclusiveIO::genericFilePrint( std::forward<>(args)...);
    }

    template<typename... Args>
    static void debugError_nf(Args &&... args) {
      ExclusiveIO::genericNotFormatPrint(std::cout, color_t::FG_BRIGHT_WHITE, std::forward<Args>(args)...);
      //ExclusiveIO::genericFilePrint( std::forward<>(args)...);
    }

#pragma endregion


    template<typename T>
    static void input(T &buf) {
      lockPrint();
      std::cin >> buf;
      unlockPrint();
    }
};

#endif //C_BDD_BDD_EXCLUSIVE_IO_HPP
