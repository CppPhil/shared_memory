#pragma once
#include <string>
#include <string_view>

namespace sm {
#ifdef _WIN32
inline constexpr std::wstring_view sharedMemoryName{L"SHARED_MEMORY_NAME"};
#else
inline constexpr std::string_view ftokFilePath{"server"};
inline constexpr int              projectId{1};
#endif

class SharedMemoryIdentifier {
public:
#ifdef _WIN32
  SharedMemoryIdentifier(std::wstring name);
#else
  SharedMemoryIdentifier(std::string filePath, int projectId);
#endif

#ifdef _WIN32
  const std::wstring& name() const noexcept;
#else
  const std::string& filePath() const noexcept;

  int projectId() const noexcept;
#endif

private:
#ifdef _WIN32
  std::wstring m_name;
#else
  std::string m_filePath;
  int         m_projectId;
#endif
};
} // namespace sm
