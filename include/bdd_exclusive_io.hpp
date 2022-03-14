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
    typedef enum {
        FG_RED = 31,
        FG_GREEN = 32,
        FG_YELLOW = 33,
        FG_BRIGHT_BLUE = 94,
        FG_BLUE = 34,
        FG_BRIGHT_MAGENTA = 95,
        FG_MAGENTA = 35,
        FG_BRIGHT_CYAN = 96,
        FG_CYAN = 36,
        FG_WHITE = 37,
        FG_DEFAULT = 39,

    } color_t;

    inline static pid_t parent_pid;
    inline static synchronisation_mutex_t *synchronisation_map;
    inline static std::ofstream log_ofstream;

    static void lockPrint() {
      pthread_mutex_lock(&synchronisation_map->print_mutex);
    }

    static void unlockPrint() {
      pthread_mutex_unlock(&synchronisation_map->print_mutex);
    }

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
    genericPrint(std::ostream &out, ExclusiveIO::color_t color, const std::string_view &rt_fmt_str,
                 Args &&... args) {
      lockPrint();
      out << "\033[" << color << "m" << (isParent() ? "[BDD]: " : "[Child]: ")
          << formatArgs(rt_fmt_str, args...) << "\033[0m" << std::flush;
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

    template<typename... Args>
    static void info(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericPrint(std::cout, isParent() ? color_t::FG_BLUE : color_t::FG_GREEN, rt_fmt_str,
                                std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericPrint(std::cerr, color_t::FG_RED, rt_fmt_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void hint(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericPrint(std::cout, color_t::FG_CYAN, rt_fmt_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericPrint(std::cout, color_t::FG_MAGENTA, rt_fmt_str, std::forward<Args>(args)...);
      //ExclusiveIO::genericFilePrint(rt_fmt_str, std::forward<>(args)...);
    }

    template<typename... Args>
    static void debugError(const std::string_view &rt_fmt_str, Args &&... args) {
      ExclusiveIO::genericPrint(std::cout, color_t::FG_BRIGHT_MAGENTA, rt_fmt_str, std::forward<Args>(args)...);
      //ExclusiveIO::genericFilePrint(rt_fmt_str, std::forward<>(args)...);
    }

    template<typename T>
    static void input(T &buf) {
      lockPrint();
      std::cin >> buf;
      unlockPrint();
    }
};

#endif //C_BDD_BDD_EXCLUSIVE_IO_HPP
