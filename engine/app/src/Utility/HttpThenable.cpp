#include <future>
#include <functional>
#include <thread>

namespace Aya
{

class HttpFuture;

class HttpThenable
{
public:
    HttpThenable(std::future<std::string>&& future)
        : m_future(std::move(future))
    {
    }

    template<typename Func>
    auto then(Func&& func) -> std::future<decltype(func(std::string{}))>
    {
        std::promise<decltype(func(std::string{}))> promise;
        auto ret_future = promise.get_future();

        std::future<std::string> moved_future = std::move(m_future);
        std::thread(
            [promise = std::move(promise), moved_future = std::move(moved_future), func = std::forward<Func>(func)]() mutable
            {
                try
                {
                    auto result = moved_future.get();
                    promise.set_value(func(std::move(result)));
                }
                catch (...)
                {
                    try
                    {
                        promise.set_exception(std::current_exception());
                    }
                    catch (...)
                    {
                    }
                }
            })
            .detach();

        return ret_future;
    }

private:
    std::future<std::string> m_future;
};

HttpThenable HttpAsync::get(const std::string& url, const HttpOptions& options)
{
    std::future<std::string> future = /* your existing future from HttpAsync::get */;
    return HttpThenable(std::move(future));
}

} // namespace Aya
