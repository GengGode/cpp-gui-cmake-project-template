#include "watcher.hpp"

#include "watcher_impl.hpp"

watcher::watcher() : impl(std::make_unique<impl_t>()) {}
watcher::~watcher()
{
    impl.reset();
}
void watcher::render()
{
    impl->render();
}
void watcher::watch(std::string name, int value)
{
    impl->watch(name, value);
}
void watcher::watch(std::string name, double value)
{
    impl->watch(name, value);
}
void watcher::watch(std::string name, std::vector<int>& value)
{
    impl->watch(name, value);
}
void watcher::watch(std::string name, std::vector<double>& value)
{
    impl->watch(name, value);
}