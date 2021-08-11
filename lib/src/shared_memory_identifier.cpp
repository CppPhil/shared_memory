#include <utility>

#include "shared_memory_identifier.hpp"

namespace sm {
#ifdef _WIN32
SharedMemoryIdentifier::SharedMemoryIdentifier(std::wstring name)
  : m_name{std::move(name)}
{
  m_name = L"Global\\" + m_name;
}
#else
SharedMemoryIdentifier::SharedMemoryIdentifier(
  std::string filePath,
  int         projectId)
  : m_filePath{std::move(filePath)}, m_projectId{projectId}
{
}
#endif

#ifdef _WIN32
const std::wstring& SharedMemoryIdentifier::name() const noexcept
{
  return m_name;
}
#else

const std::string& SharedMemoryIdentifier::filePath() const noexcept
{
  return m_filePath;
}

int SharedMemoryIdentifier::projectId() const noexcept
{
  return m_projectId;
}
#endif
} // namespace sm
