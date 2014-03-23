#ifndef GG_SCOPE_CALLBACK_HPP_INCLUDED
#define GG_SCOPE_CALLBACK_HPP_INCLUDED

#include <functional>

namespace gg
{
    class scope_callback
    {
        std::function<void()> m_func;

    public:
        scope_callback();
        scope_callback(std::function<void()>);
        scope_callback(const scope_callback&) = delete;
        scope_callback(scope_callback&&) = delete;
        ~scope_callback();
        scope_callback& operator= (std::function<void()>);
        void reset();
    };
};

#endif // GG_SCOPE_CALLBACK_HPP_INCLUDED
