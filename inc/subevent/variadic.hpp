#ifndef SUBEVENT_VARIADIC_HPP
#define SUBEVENT_VARIADIC_HPP

#include <subevent/std.hpp>

SEV_NS_BEGIN

template<typename ... ParamTypes>
class Variadic
{
public:
    SEV_DECL explicit Variadic(const ParamTypes& ... params)
    {
        setParams(params ...);
    }

    SEV_DECL virtual ~Variadic()
    {
    }

public:
    SEV_DECL void setParams(const ParamTypes& ... params)
    {
        mParams = std::make_tuple(params ...);
    }

    SEV_DECL void getParams(ParamTypes& ... params) const
    {
        std::tie(params ...) = mParams;
    }

    template<size_t index, typename ParamType>
    SEV_DECL void setParam(const ParamType& param)
    {
        std::get<index>(mParams) = param;
    }

#if (SEV_CPP_VER >= 14)
    template<size_t index>
    SEV_DECL auto& getParam() const
    {
        return std::get<index>(mParams);
    }
#endif

private:
    std::tuple<ParamTypes ...> mParams;
};

SEV_NS_END

#endif // SUBEVENT_VARIADIC_HPP
