#pragma once
#include <map>
#include <memory>
#include <mutex>

namespace global
{
    namespace _detail
    {
        template <typename T> class global_variables
        {
            static inline std::map<uint64_t, std::shared_ptr<T>> instances;
            static inline std::mutex instance_mutex;

        public:
            static uint64_t cast(T* instance) { return reinterpret_cast<uint64_t>(instance); }
            static uint64_t cast(const std::shared_ptr<T>& instance) { return cast(instance.get()); }

        public:
            static std::string format()
            {
                std::map<uint64_t, std::shared_ptr<T>> snapshot;
                {
                    std::lock_guard<std::mutex> lock(instance_mutex);
                    snapshot = instances;
                }

                std::string result = "Global Variables: " + std::string(typeid(T).name()) + " { ";
                for (const auto& [id, instance] : snapshot)
                {
                    result += std::to_string(id) + ": " + (instance ? "valid" : "null");
                    if (id != snapshot.rbegin()->first)
                        result += ", ";
                }
                result += " }";
                return result;
            }

        public:
            static std::shared_ptr<T> get(uint64_t id)
            {
                std::lock_guard<std::mutex> lock(instance_mutex);
                auto it = instances.find(id);
                if (it != instances.end())
                    return it->second;
                return nullptr;
            }
            static void set(std::shared_ptr<T> instance)
            {
                std::lock_guard<std::mutex> lock(instance_mutex);
                instances[cast(instance)] = instance;
            }

            template <typename... Args> static uint64_t create(Args&&... args)
            {
                std::lock_guard<std::mutex> lock(instance_mutex);
                auto instance = std::make_shared<T>(std::forward<Args>(args)...);
                auto id = cast(instance);
                instances[id] = instance;
                return id;
            }
            template <typename... Args> static std::pair<uint64_t, std::shared_ptr<T>> create_and_get(Args&&... args)
            {
                std::lock_guard<std::mutex> lock(instance_mutex);
                auto instance = std::make_shared<T>(std::forward<Args>(args)...);
                auto id = cast(instance);
                instances[id] = instance;
                return { id, instance };
            }

            static void destroy(uint64_t id)
            {
                std::lock_guard<std::mutex> lock(instance_mutex);
                instances.erase(id);
            }
            static void destroy(const std::shared_ptr<T>& instance) { destroy(cast(instance)); }
        };
    } // namespace _detail
    template <typename T> static inline uint64_t cast(T* instance)
    {
        return _detail::global_variables<T>::cast(instance);
    }
    template <typename T> static inline uint64_t cast(const std::shared_ptr<T>& instance)
    {
        return _detail::global_variables<T>::cast(instance);
    }
    template <typename T> static inline std::string format()
    {
        return _detail::global_variables<T>::format();
    }
    template <typename T> static inline std::shared_ptr<T> get(uint64_t id)
    {
        return _detail::global_variables<T>::get(id);
    }
    template <typename T> static inline void set(std::shared_ptr<T> instance)
    {
        _detail::global_variables<T>::set(instance);
    }
    template <typename T, typename... Args> static inline uint64_t create(Args&&... args)
    {
        return _detail::global_variables<T>::create(std::forward<Args>(args)...);
    }
    template <typename T, typename... Args> static inline std::pair<uint64_t, std::shared_ptr<T>> create_and_get(Args&&... args)
    {
        return _detail::global_variables<T>::create_and_get(std::forward<Args>(args)...);
    }
    template <typename T> static inline void destroy(uint64_t id)
    {
        _detail::global_variables<T>::destroy(id);
    }
    template <typename T> static inline void destroy(const std::shared_ptr<T>& instance)
    {
        _detail::global_variables<T>::destroy(instance);
    }

    namespace onlyone
    {
        namespace _detail
        {
            namespace types
            {
                template <class F> struct function_traits
                {
                private:
                    using call_type = function_traits<decltype(&F::operator())>;

                public:
                    using return_type = typename call_type::return_type;
                    using flat_func_t = typename call_type::flat_func_t;
                    using this_t = void;

                    static const std::size_t arity = call_type::arity;

                    template <std::size_t N> struct argument
                    {
                        static_assert(N < arity, "error: invalid parameter index.");
                        using type = typename call_type::template argument<N>::type;
                    };
                };

                template <class R, class... Args> struct function_traits<R (*)(Args...)> : public function_traits<R(Args...)>
                {
                };

                template <class R, class... Args> struct function_traits<R(Args...)>
                {
                    using return_type = R;
                    using flat_func_t = R(Args...);
                    using this_t = void;

                    static const std::size_t arity = sizeof...(Args);

                    template <std::size_t N> struct argument
                    {
                        static_assert(N < arity, "error: invalid parameter index.");
                        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
                    };
                };

                // member function pointer
                template <class C, class R, class... Args> struct function_traits<R (C::*)(Args...)> : public function_traits<R(Args...)>
                {
                    using this_t = C;
                };

                // const member function pointer
                template <class C, class R, class... Args> struct function_traits<R (C::*)(Args...) const> : public function_traits<R(Args...)>
                {
                    using this_t = C;
                };

                // member object pointer
                template <class C, class R> struct function_traits<R(C::*)> : public function_traits<R(void)>
                {
                    using this_t = C;
                };

                template <class F> struct function_traits<F&> : public function_traits<F>
                {
                };

                template <class F> struct function_traits<F&&> : public function_traits<F>
                {
                };
            } // namespace types

            template <typename T> class global_variable
            {
                static inline std::shared_ptr<T> instance;
                static inline std::mutex instance_mutex;

            public:
                static std::string format()
                {
                    std::lock_guard<std::mutex> lock(instance_mutex);
                    return "Global Instance: + " + std::string(typeid(T).name()) + " { " + (instance ? "valid" : "null") + " }";
                }

                static std::shared_ptr<T> get()
                {
                    std::lock_guard<std::mutex> lock(instance_mutex);
                    return instance;
                }
                static void set(std::shared_ptr<T> inst)
                {
                    std::lock_guard<std::mutex> lock(instance_mutex);
                    instance = inst;
                }
                template <typename... Args> static std::shared_ptr<T> create(Args&&... args)
                {
                    std::lock_guard<std::mutex> lock(instance_mutex);
                    instance = std::make_shared<T>(std::forward<Args>(args)...);
                    return instance;
                }
                static void destroy()
                {
                    std::lock_guard<std::mutex> lock(instance_mutex);
                    instance.reset();
                }

                template <typename Func, typename... Args> static auto call(Func&& func, Args&&... args)
                {
                    using first_arg_type = typename types::function_traits<Func>::template argument<0>::type;
                    static_assert(std::is_same_v<first_arg_type, T&>, "Function must accept the instance reference type as the first argument.");

                    std::lock_guard<std::mutex> lock(instance_mutex);
                    if (!instance)
                        return std::invoke_result_t<Func, T&, Args...>{};
                    return std::invoke(std::forward<Func>(func), *instance, std::forward<Args>(args)...);
                }
            };
        } // namespace _detail

        template <typename T> static inline std::string format()
        {
            return _detail::global_variable<T>::format();
        }
        template <typename T> static inline std::shared_ptr<T> get()
        {
            return _detail::global_variable<T>::get();
        }
        template <typename T> static inline void set(std::shared_ptr<T> instance)
        {
            _detail::global_variable<T>::set(instance);
        }
        template <typename T, typename... Args> static inline std::shared_ptr<T> create(Args&&... args)
        {
            return _detail::global_variable<T>::create(std::forward<Args>(args)...);
        }
        template <typename T> static inline void destroy()
        {
            _detail::global_variable<T>::destroy();
        }
        template <typename T, typename Func, typename... Args> static inline auto call(Func&& func, Args&&... args)
        {
            return _detail::global_variable<T>::call(std::forward<Func>(func), std::forward<Args>(args)...);
        }
    } // namespace onlyone
} // namespace global