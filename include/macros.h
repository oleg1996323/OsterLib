#pragma once
#include <unordered_set>
#include <memory>
#define DEFINE_SMART_POINTER_HASH_METHODS(TYPE)\
template<>\
struct std::hash<std::shared_ptr<TYPE>>\
{\
    using is_transparent = std::true_type;\
    size_t operator()(const std::shared_ptr<TYPE>& node_ptr) const{\
        return std::hash<TYPE>{}(*node_ptr);\
    }\
    size_t operator()(const TYPE& node_ptr) const{\
        return std::hash<TYPE>{}(node_ptr);\
    }\
};\
\
template<>\
struct std::hash<std::weak_ptr<TYPE>>\
{\
    using is_transparent = std::true_type;\
    size_t operator()(const std::weak_ptr<TYPE>& node_ptr) const{\
        return std::hash<TYPE>{}(*node_ptr.lock());\
    }\
    size_t operator()(const TYPE& node_ptr) const{\
        return std::hash<TYPE>{}(node_ptr);\
    }\
};\
\
template<>\
struct std::equal_to<std::shared_ptr<TYPE>>\
{\
    using is_transparent = std::true_type;\
    bool operator()(const std::shared_ptr<TYPE>& lhs, const std::shared_ptr<TYPE>& rhs) const{\
        return *lhs==*rhs;\
    }\
    bool operator()(const std::weak_ptr<TYPE>& lhs, const std::shared_ptr<TYPE>& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==*rhs;\
    }\
    bool operator()(const std::shared_ptr<TYPE>& lhs, const std::weak_ptr<TYPE>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return *lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE>& lhs, const std::weak_ptr<TYPE>& rhs) const{\
        if(rhs.expired() || lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==*rhs.lock();\
    }\
    bool operator()(const TYPE& lhs, const std::weak_ptr<TYPE>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE>& lhs, const TYPE& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==rhs;\
    }\
    bool operator()(const std::shared_ptr<TYPE>& lhs, const TYPE& rhs) const{\
            return *lhs==rhs;\
    }\
    bool operator()(const TYPE& lhs, const std::shared_ptr<TYPE>& rhs) const{\
        return lhs==*rhs;\
}\
};\
\
template<>\
struct std::equal_to<std::weak_ptr<TYPE>>\
{\
    using is_transparent = std::true_type;\
    bool operator()(const std::weak_ptr<TYPE>& lhs, const std::shared_ptr<TYPE>& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==*rhs;\
    }\
    bool operator()(const std::shared_ptr<TYPE>& lhs, const std::weak_ptr<TYPE>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return *lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE>& lhs, const std::weak_ptr<TYPE>& rhs) const{\
        if(rhs.expired() || lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==*rhs.lock();\
    }\
    bool operator()(const TYPE& lhs, const std::weak_ptr<TYPE>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE>& lhs, const TYPE& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==rhs;\
    }\
};\
\
template<>\
struct std::equal_to<TYPE>\
{\
    using is_transparent = std::true_type;\
    \
    bool operator()(const TYPE& lhs, const std::weak_ptr<TYPE>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE>& lhs, const TYPE& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==rhs;\
    }\
    bool operator()(const std::shared_ptr<TYPE>& lhs, const TYPE& rhs) const{\
            return *lhs==rhs;\
    }\
    bool operator()(const TYPE& lhs, const std::shared_ptr<TYPE>& rhs) const{\
        return lhs==*rhs;\
    }\
    bool operator()(const TYPE& lhs, const TYPE& rhs) const{\
        return lhs==rhs;\
    }\
};


#define DEFINE_SMART_POINTER_HASH_METHODS_TEMPLATED(TYPE,TEMPLATE_ARG,...) \
template<typename TEMPLATE_ARG __VA_OPT__(,typename ) __VA_ARGS__>\
struct std::hash<std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>>\
{\
    using is_transparent = std::true_type;\
    size_t operator()(const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& node_ptr) const{\
        return std::hash<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>{}(*node_ptr);\
    }\
    size_t operator()(const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& node_ptr) const{\
        return std::hash<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>{}(node_ptr);\
    }\
};\
\
template<typename TEMPLATE_ARG __VA_OPT__(,typename ) __VA_ARGS__>\
struct std::hash<std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>>\
{\
    using is_transparent = std::true_type;\
    size_t operator()(const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& node_ptr) const{\
        return std::hash<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>{}(*node_ptr.lock());\
    }\
    size_t operator()(const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& node_ptr) const{\
        return std::hash<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>{}(node_ptr);\
    }\
};\
\
template<typename TEMPLATE_ARG __VA_OPT__(,typename ) __VA_ARGS__>\
struct std::equal_to<std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>>\
{\
    using is_transparent = std::true_type;\
    bool operator()(const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        return *lhs==*rhs;\
    }\
    bool operator()(const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==*rhs;\
    }\
    bool operator()(const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return *lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(rhs.expired() || lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==*rhs.lock();\
    }\
    bool operator()(const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& lhs, const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==rhs;\
    }\
    bool operator()(const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& rhs) const{\
            return *lhs==rhs;\
    }\
    bool operator()(const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& lhs, const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        return lhs==*rhs;\
}\
};\
\
template<typename TEMPLATE_ARG __VA_OPT__(,typename ) __VA_ARGS__>\
struct std::equal_to<std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>>\
{\
    using is_transparent = std::true_type;\
    bool operator()(const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==*rhs;\
    }\
    bool operator()(const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return *lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(rhs.expired() || lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==*rhs.lock();\
    }\
    bool operator()(const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& lhs, const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==rhs;\
    }\
};\
\
template<typename TEMPLATE_ARG __VA_OPT__(,typename ) __VA_ARGS__>\
struct std::equal_to<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>\
{\
    using is_transparent = std::true_type;\
    \
    bool operator()(const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& lhs, const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        if(rhs.expired())\
            return false;\
        else\
            return lhs==*rhs.lock();\
    }\
    bool operator()(const std::weak_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& rhs) const{\
        if(lhs.expired())\
            return false;\
        else\
            return *lhs.lock()==rhs;\
    }\
    bool operator()(const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& lhs, const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& rhs) const{\
            return *lhs==rhs;\
    }\
    bool operator()(const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& lhs, const std::shared_ptr<TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>>& rhs) const{\
        return lhs==*rhs;\
    }\
    bool operator()(const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& lhs, const TYPE<TEMPLATE_ARG __VA_OPT__(,)__VA_ARGS__>& rhs) const{\
        return lhs==rhs;\
    }\
};

struct macros
{
    int i;
    /* data */
};


