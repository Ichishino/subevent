#ifndef SUBEVENT_VARIADIC_HPP
#define SUBEVENT_VARIADIC_HPP

#include <subevent/std.hpp>

SEV_NS_BEGIN

template<typename ... ParamTypes>
class Variadic
{
public:
    explicit Variadic(const ParamTypes& ... params)
    {
        setParams(params ...);
    }

    virtual ~Variadic()
    {
    }

public:
    void setParams(const ParamTypes& ... params)
    {
        mParams = std::make_tuple(params ...);
    }

    void getParams(ParamTypes& ... params) const
    {
        std::tie(params ...) = mParams;
    }

    template<size_t index, typename ParamType>
    void setParam(const ParamType& param)
    {
        std::get<index>(mParams) = param;
    }

#if (__cplusplus >= 201402L) || (defined(_MSC_VER) && _MSC_VER >= 1900)
    template<size_t index>
    auto& getParam() const
    {
        return std::get<index>(mParams);
    }
#endif

private:
    std::tuple<ParamTypes ...> mParams;
};

SEV_NS_END

#endif // SUBEVENT_VARIADIC_HPP
