#pragma once
// Copyright(c) 2025 Geng Gode
//
// Signal/Slot mechanism for event-driven UI

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

template <typename... Args> struct signal : public std::enable_shared_from_this<signal<Args...>>
{
private:
    signal() = default;

public:
    using slot_t = std::function<void(Args...)>;
    static std::shared_ptr<signal> create() { return std::shared_ptr<signal>(new signal()); }

private:
    struct entry
    {
        slot_t fn;
        bool alive = true;
    };

public:
    struct connection
    {
        connection() = default;
        void disconnect()
        {
            if (auto s = slot_entry.lock())
                s->alive = false;
            if (auto sig = signal_ref.lock())
                sig->request_sweep();
        }
        bool connected() const
        {
            if (auto s = slot_entry.lock())
                return s->alive;
            return false;
        }

    private:
        std::weak_ptr<signal> signal_ref;
        std::weak_ptr<entry> slot_entry;
        friend struct signal;
        connection(std::shared_ptr<signal> sig, std::shared_ptr<entry> entry) : signal_ref(sig), slot_entry(entry) {}
    };

    connection connect(slot_t slot)
    {
        auto slot_entry = std::make_shared<entry>(std::move(slot));
        slots.push_back(slot_entry);
        return connection(this->shared_from_this(), slot_entry);
    }

    void disconnect_all()
    {
        for (auto& s : slots)
            s->alive = false;
        request_sweep();
    }

    void emit(Args... args) const
    {
        ++emitting_count;
        for (const auto& s : slots)
            if (s->alive)
                s->fn(args...);
        --emitting_count;
        if (emitting_count == 0 && pending_sweep)
            const_cast<signal*>(this)->sweep();
    }

    void operator()(Args... args) const { emit(args...); }

    bool empty() const { return slots.empty(); }

private:
    void request_sweep() const
    {
        ++dead_count;
        if (emitting_count > 0)
            pending_sweep = true;
        else if (dead_count < static_cast<int>(slots.size()) >> 2)
            pending_sweep = true;
        else
            const_cast<signal*>(this)->sweep();
    }
    void sweep()
    {
        dead_count = 0;
        pending_sweep = false;
        slots.erase(std::remove_if(slots.begin(), slots.end(), [](auto& s) { return !s || !s->alive; }), slots.end());
    }
    std::vector<std::shared_ptr<entry>> slots;
    mutable int emitting_count = 0;
    mutable int dead_count = 0;
    mutable bool pending_sweep = false;
};

template <typename Connection> struct scoped_connection
{
    scoped_connection(Connection conn) : conn(std::move(conn)) {}
    ~scoped_connection() { conn.disconnect(); }
    scoped_connection(const scoped_connection&) = delete;
    scoped_connection& operator=(const scoped_connection&) = delete;
    scoped_connection(scoped_connection&& other) noexcept : conn(std::move(other.conn)) {}
    scoped_connection& operator=(scoped_connection&& other) noexcept
    {
        if (this != &other)
            conn.disconnect();
        conn = std::move(other.conn);
        return *this;
    }

    void disconnect() { conn.disconnect(); }
    bool connected() const { return conn.connected(); }

private:
    Connection conn;
};
