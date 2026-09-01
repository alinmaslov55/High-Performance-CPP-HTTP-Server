#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <sys/epoll.h>

#include <vector>

namespace http{

class Epoll {
public:
    Epoll();
    ~Epoll();

    Epoll(const Epoll &) = delete;
    Epoll &operator=(const Epoll &) = delete;
    Epoll(Epoll &&) = delete;
    Epoll &operator=(Epoll &&) = delete;

    void add(int fd, uint32_t events);
    void modify(int fd, uint32_t events);
    void remove(int fd);

    [[nodiscard]]
    std::vector<epoll_event> wait(int timeoutMs = -1);

private:
    int epoll_fd_;
    static constexpr int MAX_EVENTS = 1024;
};

} // namespace http

#endif // EPOLL_HPP