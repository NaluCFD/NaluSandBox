#ifndef nalu_make_unique_h
#define nalu_make_unique_h

namespace sierra {
namespace naluUnit {
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}
}

#endif
