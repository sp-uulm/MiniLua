#ifndef MINILUA_IO_HPP
#define MINILUA_IO_HPP

#include <iostream>
#include <istream>

#include "MiniLua/values.hpp"

namespace minilua {

auto create_io_table(MemoryAllocator* allocator) -> Table;

namespace io {

auto open(const CallContext& ctx) -> Vallist;
auto close(const CallContext& ctx) -> CallResult;

auto type(const CallContext& ctx) -> CallResult;

/**
 * @brief Base class for file handles used by the io implementation.
 *
 * Currently all files opened by `io.open` use `CFileHandle`.
 *
 * \todo Allow the user of the library to overwrite default file open behaviour.
 *
 * \todo Implement stdin/stdout/stderr streams using another derived class that
 * uses C++ streams.
 *
 * The following methods have to be provided by a derived class:
 *
 * - `is_open`
 * - have the same behaviour described in
 *   https://www.lua.org/manual/5.3/manual.html#6.8
 *   - `close`
 *   - `flush`
 * - arguments will be parsed by the base class but you have to implement the
 *   behaviour
 *   - `seek_impl`
 *   - `setvbuf_impl`
 * - These methods are used by `read` and correspond to the format arguments.
 *   Note these may be called multiple times for one read if there are multiple
 *   format arguments.
 *   - `read_all`
 *   - `read_num`
 *   - `read_line`
 *   - `read_line_with_newline`
 *   - `read_count`
 * - these method will be used by `write`
 *   - `write_string`
 */
class FileHandle {
public:
    enum SeekWhence { SET, CURRENT, END };
    enum SetvbufMode { NO, FULL, LINE };

    FileHandle();
    virtual ~FileHandle();

    void ensure_file_is_open();

    auto read(const CallContext& ctx) -> Vallist;

    // file:seek([whence [, offset]]])
    auto seek(const CallContext& ctx) -> Vallist;

    auto write(const CallContext& ctx) -> Vallist;

    auto lines(const CallContext& ctx) -> Value;

    auto type(const CallContext& /*ctx*/) -> Value;

    // file:setvbuf(mode, [, size])
    auto setvbuf(const CallContext& ctx) -> Value;

    virtual auto is_open() -> bool = 0;

    virtual auto close() -> bool = 0;
    virtual auto flush() -> bool = 0;
    /**
     * Returns the current position after the seek.
     */
    virtual auto seek_impl(SeekWhence whence, long offset) -> long = 0;
    /**
     * Returns true on success.
     */
    virtual auto setvbuf_impl(SetvbufMode mode, size_t size) -> bool = 0;

    virtual auto read_all() -> Value = 0;
    virtual auto read_num() -> Value = 0;
    virtual auto read_line() -> Value = 0;
    virtual auto read_line_with_newline() -> Value = 0;
    virtual auto read_count(long count) -> Value = 0;

    virtual void write_string(std::string str) = 0;
};

} // namespace io

} // namespace minilua

#endif
