#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#pragma comment(lib, "Shell32.lib")
#endif

namespace fs = std::filesystem;

static void printUsage(const char* exe) {
  std::cout
      << "Usage:\n"
      << "  " << exe
      << " [input_html] [output_html]\n\n"
      << "If output_html is omitted or set to '-', the program writes to stdout.\n"
      << "Examples:\n"
      << "  " << exe << " SaintAltarLight_Standalone.html out.html\n"
      << "  " << exe << " SaintAltarLight_Standalone.html > out.html\n";
}

#if defined(_WIN32)
static void printUsageW(const wchar_t* exe) {
  std::wcout
      << L"Usage:\n"
      << L"  " << exe
      << L" [input_html] [output_html]\n\n"
      << L"If output_html is omitted or set to '-', the program writes to stdout.\n"
      << L"Examples:\n"
      << L"  " << exe << L" SaintAltarLight_Standalone.html out.html\n"
      << L"  " << exe << L" SaintAltarLight_Standalone.html > out.html\n";
}
#endif

static int copyStream(std::istream& in, std::ostream& out) {
  std::vector<char> buffer(64 * 1024);
  while (in) {
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const auto got = in.gcount();
    if (got > 0) {
      out.write(buffer.data(), got);
      if (!out) return 3;
    }
  }
  return 0;
}

static fs::path resolveDefaultInput(const fs::path& exePath) {
  const fs::path exeDir = exePath.has_parent_path() ? exePath.parent_path() : fs::current_path();
  const fs::path repoDir = exeDir / "..";
  const fs::path candidateA = exeDir / "SaintAltarLight_Standalone.html";
  const fs::path candidateB = repoDir / "SaintAltarLight_Standalone.html";
  if (fs::exists(candidateA)) return candidateA;
  if (fs::exists(candidateB)) return candidateB;
  return candidateB;
}

static fs::path resolveDefaultOutput(const fs::path& exePath) {
  const fs::path exeDir = exePath.has_parent_path() ? exePath.parent_path() : fs::current_path();
  const fs::path repoDir = exeDir / "..";
  return repoDir / "out.html";
}

static void waitForEnterIfDoubleClicked(bool isDoubleClickMode) {
  if (!isDoubleClickMode) return;
  std::cout << "\nPress Enter to exit...";
  std::cout.flush();
  std::string dummy;
  std::getline(std::cin, dummy);
}

#if defined(_WIN32)
static void openInBrowser(const fs::path& htmlPath) {
  const std::wstring p = htmlPath.wstring();
  HINSTANCE r = ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  (void)r;
}
#endif

static int writeToFileAtomically(std::istream& in, const fs::path& outputPath) {
  fs::path tmpPath = outputPath;
  tmpPath += ".tmp";

  {
    std::ofstream out(tmpPath, std::ios::binary);
    if (!out) {
      std::cerr << "Failed to open output HTML: " << tmpPath.string() << "\n";
      return 4;
    }
    const int code = copyStream(in, out);
    if (code != 0) return code;
    out.flush();
    if (!out) return 3;
  }

  std::error_code ec;
  fs::remove(outputPath, ec);
  ec.clear();
  fs::rename(tmpPath, outputPath, ec);
  if (ec) {
    std::cerr << "Failed to rename temp output: " << ec.message() << "\n";
    return 5;
  }
  return 0;
}

static int runWithNarrowArgs(int argc, char** argv) {
  const bool isDoubleClickMode = (argc == 1);
  const fs::path exePath = fs::absolute(fs::path(argv[0] ? argv[0] : ""));

  const std::string arg1 = argc >= 2 ? std::string(argv[1]) : std::string();
  if (arg1 == "-h" || arg1 == "--help") {
    printUsage(argv[0]);
    return 0;
  }

  fs::path inputPath = argc >= 2 ? fs::path(argv[1]) : resolveDefaultInput(exePath);

  bool toStdout = false;
  fs::path outputPath;
  if (argc >= 3) {
    const std::string outArg = std::string(argv[2]);
    toStdout = (outArg == "-" || outArg.empty());
    if (!toStdout) outputPath = fs::path(outArg);
  } else {
    if (isDoubleClickMode) {
      toStdout = false;
      outputPath = resolveDefaultOutput(exePath);
    } else {
      toStdout = true;
    }
  }

  if (!fs::exists(inputPath)) {
    std::cerr << "Input HTML not found: " << inputPath.string() << "\n";
    printUsage(argv[0]);
    waitForEnterIfDoubleClicked(isDoubleClickMode);
    return 2;
  }

  std::ifstream in(inputPath, std::ios::binary);
  if (!in) {
    std::cerr << "Failed to open input HTML: " << inputPath.string() << "\n";
    waitForEnterIfDoubleClicked(isDoubleClickMode);
    return 2;
  }

  if (toStdout) {
    std::cout.sync_with_stdio(false);
    const int code = copyStream(in, std::cout);
    waitForEnterIfDoubleClicked(isDoubleClickMode);
    return code;
  }

  if (outputPath.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(outputPath.parent_path(), ec);
  }

  const int code = writeToFileAtomically(in, outputPath);
  if (code != 0) {
    waitForEnterIfDoubleClicked(isDoubleClickMode);
    return code;
  }

  std::cout << "Generated: " << outputPath.string() << "\n";

#if defined(_WIN32)
  if (isDoubleClickMode) openInBrowser(outputPath);
#endif

  waitForEnterIfDoubleClicked(isDoubleClickMode);
  return 0;
}

#if defined(_WIN32)
static int runWithWideArgs(int argc, wchar_t** argv) {
  const bool isDoubleClickMode = (argc == 1);
  const fs::path exePath = fs::absolute(fs::path(argv[0] ? argv[0] : L""));

  const std::wstring arg1 = argc >= 2 ? std::wstring(argv[1]) : std::wstring();
  if (arg1 == L"-h" || arg1 == L"--help") {
    printUsageW(argv[0]);
    return 0;
  }

  fs::path inputPath = argc >= 2 ? fs::path(argv[1]) : resolveDefaultInput(exePath);

  bool toStdout = false;
  fs::path outputPath;
  if (argc >= 3) {
    const std::wstring outArg = std::wstring(argv[2]);
    toStdout = (outArg == L"-" || outArg.empty());
    if (!toStdout) outputPath = fs::path(argv[2]);
  } else {
    if (isDoubleClickMode) {
      toStdout = false;
      outputPath = resolveDefaultOutput(exePath);
    } else {
      toStdout = true;
    }
  }

  if (!fs::exists(inputPath)) {
    std::wcerr << L"Input HTML not found: " << inputPath.wstring() << L"\n";
    printUsageW(argv[0]);
    waitForEnterIfDoubleClicked(isDoubleClickMode);
    return 2;
  }

  std::ifstream in(inputPath, std::ios::binary);
  if (!in) {
    std::wcerr << L"Failed to open input HTML: " << inputPath.wstring() << L"\n";
    waitForEnterIfDoubleClicked(isDoubleClickMode);
    return 2;
  }

  if (toStdout) {
    std::cout.sync_with_stdio(false);
    const int code = copyStream(in, std::cout);
    waitForEnterIfDoubleClicked(isDoubleClickMode);
    return code;
  }

  if (outputPath.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(outputPath.parent_path(), ec);
  }

  const int code = writeToFileAtomically(in, outputPath);
  if (code != 0) {
    waitForEnterIfDoubleClicked(isDoubleClickMode);
    return code;
  }

  std::wcout << L"Generated: " << outputPath.wstring() << L"\n";

  if (isDoubleClickMode) openInBrowser(outputPath);

  waitForEnterIfDoubleClicked(isDoubleClickMode);
  return 0;
}
#endif

#if defined(_WIN32)
int wmain(int argc, wchar_t** argv) {
  try {
    return runWithWideArgs(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
#else
int main(int argc, char** argv) {
  try {
    return runWithNarrowArgs(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
#endif
