#pragma once
#include <atomic>
#include <memory>
#include <mutex>

#include <syncer.hpp>

namespace stdex
{
    template <typename T> class channel
    {
        using syncer_t = stdex::syncer<T, true>;

    public:
        class inlet
        {
            std::weak_ptr<syncer_t> syncer_ref;
            syncer_t& source;

        public:
            inlet(std::shared_ptr<syncer_t> syncer_ptr) : syncer_ref(syncer_ptr), source(*syncer_ptr) {}
            bool set(const T& value) noexcept
            {
                if (syncer_ref.expired())
                    return false;
                source.set(value);
                return true;
            }
        };

    private:
        T value;
        std::shared_ptr<syncer_t> syncer_impl = std::make_shared<syncer_t>();
        syncer_t& syncer = *syncer_impl;

    public:
        inlet get_input() { return syncer_impl; }
        T& ref()
        {
            syncer.try_sync(value);
            return value;
        }
    };

    template <typename T> class channel_accessor
    {
        T& value;

    public:
        channel_accessor(channel<T>& channel) : value(channel.ref()) {}
        operator T&() { return value; }
    };
} // namespace stdex