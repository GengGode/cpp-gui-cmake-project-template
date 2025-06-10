
#pragma once
#include <memory>
#include <string>
#include <vector>

class watcher
{
    struct impl_t;
    std::unique_ptr<impl_t> impl;

public:
    watcher();
    ~watcher();
    void render();
    void watch(std::string name, int value);
    void watch(std::string name, double value);
    void watch(std::string name, std::vector<int>& value);
    void watch(std::string name, std::vector<double>& value);
};
