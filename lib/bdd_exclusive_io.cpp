//
// Created by byjtew on 14/03/2022.
//

#include "bdd_exclusive_io.hpp"
#include <string>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <execution>


#pragma region Private API

void ExclusiveIO::mmapExclusionStructure() {
  unlink(EXCLUSIVE_IO_MMAP_NAME);
  auto fd = open(EXCLUSIVE_IO_MMAP_NAME, O_CREAT | O_RDWR, 00600);
  ftruncate(fd, sizeof(synchronisation_mutex_t));
  synchronisation_map = (synchronisation_mutex_t *) mmap(nullptr,
                                                         sizeof(synchronisation_mutex_t),
                                                         PROT_READ | PROT_WRITE,
                                                         MAP_SHARED,
                                                         fd,
                                                         0);
}

void ExclusiveIO::createLogFile() {
  std::string log_filename;
  log_filename.resize(32);
  time_t t = time(nullptr);
  struct tm tm = *localtime(&t);
  snprintf(log_filename.data(), log_filename.size(), "./logs-%d-%02d-%02d %02d:%02d:%02d.log", tm.tm_year + 1900,
           tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  getLogStream().open(log_filename);
  if (getLogStream().fail())
    std::cerr << "log_fp creation error" << std::endl;
}


#pragma endregion


#pragma region Public API

void ExclusiveIO::initialize(pid_t parentPid) {
  parent_pid = parentPid;

  mmapExclusionStructure();

  pthread_mutexattr_t mutexattr;
  if (pthread_mutexattr_init(&mutexattr))
    throw std::invalid_argument("pthread_mutexattr_init");
  if (pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED))
    throw std::invalid_argument("pthread_mutexattr_setpshared");

  createLogFile();

  if (pthread_mutex_init(&synchronisation_map->print_mutex, &mutexattr))
    throw std::invalid_argument("pthread_mutex_init for print_mutex");
  if (pthread_mutex_init(&synchronisation_map->log_print_mutex, &mutexattr))
    throw std::invalid_argument("pthread_mutex_init for log_print_mutex");

}

void ExclusiveIO::terminate() {
  std::cout << std::flush;
  munmap(synchronisation_map, sizeof(synchronisation_mutex_t));
  getLogStream().close();
}


#pragma endregion



