#pragma once
// Copyright(c) 2025 Geng Gode
//
// Signal/Slot mechanism for event-driven UI

#include <algorithm>
#include <functional>
#include <vector>

template <typename... Args> struct signal
{
    using slot_t = std::function<void(Args...)>;

    int connect(slot_t slot)
    {
        int id = next_id++;
        slots.push_back({ id, std::move(slot) });
        return id;
    }

    void disconnect(int id)
    {
        std::erase_if(slots, [id](auto& s) { return s.id == id; });
    }

    void emit(Args... args) const
    {
        for (auto& s : slots)
            s.fn(args...);
    }

    void operator()(Args... args) const { emit(args...); }

    bool empty() const { return slots.empty(); }

private:
    struct entry
    {
        int id;
        slot_t fn;
    };
    std::vector<entry> slots;
    int next_id = 0;
};