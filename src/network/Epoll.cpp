#include "network/Epoll.hpp"

#include <stdexcept>
#include <unistd.h>

namespace http{

Epoll::Epoll(){
    epoll_fd_ = ::epoll_create1(0);
    if(epoll_fd_ == -1){
        throw std::runtime_error("Failed to create epoll instance");
    }
}

Epoll::~Epoll(){
    if(epoll_fd_ != -1){
        ::close(epoll_fd_);
    }
}

void Epoll::add(int fd, uint32_t events){
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;

    if(::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1){
        throw std::runtime_error("Failed to add file descriptor to epoll");
    }
}
void Epoll::modify(int fd, uint32_t events){
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) == -1) {
        throw std::runtime_error("Failed to modify file descriptor in epoll");
    }
}
void Epoll::remove(int fd){
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        throw std::runtime_error("Failed to remove file descriptor from epoll");
    }
}

std::vector<epoll_event> Epoll::wait(int timeoutMs = -1){
    std::vector<epoll_event> events(MAX_EVENTS);

    const int num_events = ::epoll_wait(epoll_fd_, events.data(), MAX_EVENTS, timeoutMs);

    if (num_events == -1) {
        if (errno == EINTR) {
            return {};
        }
        throw std::runtime_error("epoll_wait failed");
    }

    events.resize(static_cast<std::size_t>(num_events));

    return events;
}

} // namespace http