#include <sol/sol.hpp>

#include <future>
#include <variant>
#include <vector>


using Function = sol::protected_function;
using ReturnType = sol::protected_function_result;

class Async
{
    public:
        Async(sol::this_state s, sol::protected_function function, sol::variadic_args args)
        :m_future(std::async(std::launch::async, [](Function func, sol::variadic_args args){
            sol::state lua;
        }, function, args))
        {
        }

    private:
    std::future<ReturnType> m_future;
};

int main()
{
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    lua.new_usertype<Async>("Async", 
        sol::call_constructor, Async::Async)
}