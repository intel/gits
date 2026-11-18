// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for stopping the stream reader: by an exception of a command or by closing it
// while the reading and decompression threads have filled every block ahead of the command being
// run, and by the end of the stream or closing while the command thread waits for a block. A hang
// there blocks the player forever instead of reporting the exception or closing.

#include "streamReader.h"
#include "streamCompressor.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

namespace {

using gits::stream::CommandFactory;
using gits::stream::CommandRunner;
using gits::stream::LZ4StreamCompressor;
using gits::stream::StreamReader;

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

constexpr uint32_t kBlocks = 64;
constexpr uint32_t kStopAt = 1;

enum class StopMode {
  None,
  Throw,
  Close,
};

struct State {
  StopMode Mode{};
  StreamReader* Reader{};
  std::atomic<uint32_t> Runs{};
  std::atomic<bool> InOrder{true};
};

class Runner : public CommandRunner {
public:
  explicit Runner(State& state) : m_State(state) {}
  void Run() override {
    if (m_Index != m_State.Runs.load()) {
      m_State.InOrder = false;
    }
    ++m_State.Runs;
    if (m_Index == kStopAt && m_State.Mode != StopMode::None) {
      // Lets the reading and decompression threads fill every block before stopping.
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      if (m_State.Mode == StopMode::Throw) {
        throw std::runtime_error("command failed");
      }
      m_State.Reader->Close();
    }
  }

protected:
  void DecodeCommand() override {
    std::memcpy(&m_Index, m_Data, sizeof(m_Index));
  }

private:
  State& m_State;
  uint32_t m_Index{};
};

class Factory : public CommandFactory {
public:
  explicit Factory(State& state) : m_State(state) {}
  CommandRunner* CreateCommand(unsigned) override {
    return new Runner(m_State);
  }

private:
  State& m_State;
};

// Each block holds one command whose data is its index.
std::string MakeStream(uint32_t blocks = kBlocks) {
  LZ4StreamCompressor compressor;
  std::string stream;
  for (uint32_t index = 0; index < blocks; ++index) {
    std::string block;
    const unsigned id = 1;
    const uint64_t size = sizeof(index);
    block.append(reinterpret_cast<const char*>(&id), sizeof(id));
    block.append(reinterpret_cast<const char*>(&size), sizeof(size));
    block.append(reinterpret_cast<const char*>(&index), sizeof(index));

    std::vector<char> compressed(compressor.CompressBound(block.size()));
    const uint64_t compressedSize =
        compressor.Compress(block.data(), compressed.data(), block.size(), compressed.size());
    const uint64_t uncompressedSize = block.size();
    stream.append(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize));
    stream.append(reinterpret_cast<const char*>(&uncompressedSize), sizeof(uncompressedSize));
    stream.append(compressed.data(), compressedSize);
  }
  return stream;
}

// Holds back the end of the stream until released, as a slow disk does.
class GatedBuffer : public std::streambuf {
public:
  explicit GatedBuffer(std::string data) : m_Data(std::move(data)) {
    setg(m_Data.data(), m_Data.data(), m_Data.data() + m_Data.size());
  }
  void Release() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Released = true;
    m_Condition.notify_all();
  }

protected:
  int_type underflow() override {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Condition.wait(lock, [this] { return m_Released; });
    return traits_type::eof();
  }

private:
  std::string m_Data;
  std::mutex m_Mutex;
  std::condition_variable m_Condition;
  bool m_Released{};
};

// Threads stuck in a wait cannot be joined, so a hang ends the test process.
class Watchdog {
public:
  explicit Watchdog(const char* name) : m_Name(name), m_Thread([this] { Watch(); }) {}
  ~Watchdog() {
    m_Done = true;
    m_Thread.join();
  }

private:
  void Watch() {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (!m_Done) {
      if (std::chrono::steady_clock::now() > deadline) {
        std::printf("FAIL: %s hangs\n", m_Name);
        std::fflush(stdout);
        std::quick_exit(EXIT_FAILURE);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  const char* m_Name;
  std::atomic<bool> m_Done{};
  std::thread m_Thread;
};

void TestRunsAllCommands() {
  Watchdog watchdog("running a stream");
  State state;
  Factory factory(state);
  std::vector<CommandFactory*> factories{&factory};
  std::istringstream stream(MakeStream());
  StreamReader reader(factories, stream);
  state.Reader = &reader;
  reader.Run();
  Check(state.Runs == kBlocks, "every command of the stream runs");
  Check(state.InOrder, "the commands run in stream order");
}

void TestThrowWhileBlocksAreFull() {
  Watchdog watchdog("a command throwing while all blocks are full");
  State state;
  state.Mode = StopMode::Throw;
  Factory factory(state);
  std::vector<CommandFactory*> factories{&factory};
  std::istringstream stream(MakeStream());
  StreamReader reader(factories, stream);
  state.Reader = &reader;
  bool thrown = false;
  try {
    reader.Run();
  } catch (const std::runtime_error& error) {
    thrown = std::string(error.what()) == "command failed";
  }
  Check(thrown, "the exception of a command leaves Run");
  Check(state.Runs == kStopAt + 1, "no command runs after the one that threw");
}

void TestCloseWhileBlocksAreFull() {
  Watchdog watchdog("closing while all blocks are full");
  State state;
  state.Mode = StopMode::Close;
  Factory factory(state);
  std::vector<CommandFactory*> factories{&factory};
  std::istringstream stream(MakeStream());
  StreamReader reader(factories, stream);
  state.Reader = &reader;
  reader.Run();
  Check(state.Runs == kStopAt + 1, "no command runs after the reader is closed");
}

// The command thread waits for a block past the last one read when the stream ends, or when
// another thread closes the reader, e.g. the player window being closed.
void TestStopWhileWaitingForBlock(bool close) {
  Watchdog watchdog(close ? "closing while waiting for a block"
                          : "the stream ending while waiting for a block");
  constexpr uint32_t blocks = 4;
  State state;
  Factory factory(state);
  std::vector<CommandFactory*> factories{&factory};
  GatedBuffer buffer(MakeStream(blocks));
  std::istream stream(&buffer);
  StreamReader reader(factories, stream);
  state.Reader = &reader;
  std::thread stopper([&] {
    while (state.Runs < blocks) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    if (close) {
      reader.Close();
    }
    buffer.Release();
  });
  reader.Run();
  stopper.join();
  Check(state.Runs == blocks, "every command read before the stop runs");
}

} // namespace

int main() {
  TestRunsAllCommands();
  TestThrowWhileBlocksAreFull();
  TestCloseWhileBlocksAreFull();
  TestStopWhileWaitingForBlock(false);
  TestStopWhileWaitingForBlock(true);
  if (g_Failures) {
    std::printf("%d check(s) failed\n", g_Failures);
    return EXIT_FAILURE;
  }
  std::printf("All stream reader tests passed\n");
  return EXIT_SUCCESS;
}
