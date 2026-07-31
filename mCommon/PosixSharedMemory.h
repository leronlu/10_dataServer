#pragma once
#include <string>
#if defined(__linux__)
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#endif

class PosixSharedMemory {
public:
    PosixSharedMemory() : fd(-1), shmSize(0), shmPtr(nullptr), semHandle(nullptr) {}
    ~PosixSharedMemory() { detach(); }

    PosixSharedMemory(const PosixSharedMemory&) = delete;
    PosixSharedMemory& operator=(const PosixSharedMemory&) = delete;

    void setKey(const std::string &key) {
        name = sanitizeName(key);
        semName = name + "_sem";
    }

    bool create(size_t size) {
    #if defined(__linux__)
        fd = ::shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1) return false;
        if (::ftruncate(fd, static_cast<off_t>(size)) == -1) { ::close(fd); fd = -1; return false; }
        shmSize = size;
        shmPtr = ::mmap(nullptr, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (shmPtr == MAP_FAILED) { shmPtr = nullptr; ::close(fd); fd = -1; return false; }
        semHandle = ::sem_open(semName.c_str(), O_CREAT, 0666, 1);
        return semHandle != SEM_FAILED;
    #else
        (void)size; return false;
    #endif
    }

    bool attach() {
    #if defined(__linux__)
        fd = ::shm_open(name.c_str(), O_RDWR, 0666);
        if (fd == -1) return false;
        struct stat st{};
        if (::fstat(fd, &st) == -1) { ::close(fd); fd = -1; return false; }
        shmSize = static_cast<size_t>(st.st_size);
        shmPtr = ::mmap(nullptr, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (shmPtr == MAP_FAILED) { shmPtr = nullptr; ::close(fd); fd = -1; return false; }
        semHandle = ::sem_open(semName.c_str(), 0);
        return semHandle != SEM_FAILED;
    #else
        return false;
    #endif
    }

    bool isAttached() const { return shmPtr != nullptr; }

    void lock() {
    #if defined(__linux__)
        if (semHandle) {
            while (::sem_wait(static_cast<sem_t*>(semHandle)) == -1 && errno == EINTR) {}
        }
    #endif
    }
    void unlock() {
    #if defined(__linux__)
        if (semHandle) ::sem_post(static_cast<sem_t*>(semHandle));
    #endif
    }

    bool tryLock() {
    #if defined(__linux__)
        if (semHandle) return (::sem_trywait(static_cast<sem_t*>(semHandle)) == 0);
        return false;
    #else
        return false;
    #endif
    }

    bool lockFor(int ms) {
    #if defined(__linux__)
        if (!semHandle) return false;
        struct timespec ts{};
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += ms / 1000;
        ts.tv_nsec += (ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000L; }
        return (::sem_timedwait(static_cast<sem_t*>(semHandle), &ts) == 0);
    #else
        return false;
    #endif
    }

    void * data() { return shmPtr; }
    const void * data() const { return shmPtr; }
    size_t size() const { return shmSize; }

    void detach() {
    #if defined(__linux__)
        if (shmPtr) { ::munmap(shmPtr, shmSize); shmPtr = nullptr; }
        if (fd != -1) { ::close(fd); fd = -1; }
        if (semHandle) { ::sem_close(static_cast<sem_t*>(semHandle)); semHandle = nullptr; }
    #endif
    }

private:
    static std::string sanitizeName(const std::string &key) {
        std::string n;
        n.reserve(key.size() + 1);
        n.push_back('/');
        for (char c : key) {
            if (c == '\\' || c == '/') n.push_back('_');
            else n.push_back(c);
        }
        return n;
    }

    std::string name;
    std::string semName;
    int fd;
    size_t shmSize;
    void * shmPtr;
    void * semHandle;
};
