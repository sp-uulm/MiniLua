#include <bits/types/FILE.h>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <istream>
#include <iterator>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "MiniLua/environment.hpp"
#include "MiniLua/io.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua {

namespace io {

namespace {
class FileOpenError {
public:
    std::string message;
    int value;

    FileOpenError(std::string message, int value) : message(std::move(message)), value(value) {}
};
} // namespace

auto type(const CallContext& ctx) -> CallResult {
    auto file = ctx.arguments().get(0);

    // return the result of calling the "type" method if the file is a table
    // and the method exists, otherwise return nil
    return std::visit(
        overloaded{
            [&ctx](const Table& table) {
                if (table.get("type").is_function()) {
                    return table.get("type").call(ctx);
                } else {
                    return CallResult();
                }
            },
            [](const auto& /*unused*/) { return CallResult(); }},
        file.raw());
}

class CFileHandle : public FileHandle {
    // NOTE: a nullptr is treated as a closed file
    FILE* handle;
    bool should_close;

public:
    CFileHandle(const std::string& path, const std::string& mode)
        : CFileHandle(std::fopen(path.c_str(), mode.c_str()), true) {}
    CFileHandle(FILE* handle, bool should_close) : handle(handle), should_close(should_close) {
        if (this->handle == nullptr) {
            throw FileOpenError("could not open file", errno);
        }
    }
    ~CFileHandle() override {
        if (this->should_close && this->handle != nullptr) {
            fclose(this->handle);
            this->handle = nullptr;
        }
    }

    auto is_open() -> bool override { return this->handle != nullptr; }
    auto is_at_eof() -> bool override { return std::feof(this->handle) != 0; }

    auto close() -> bool override {
        this->ensure_file_is_open();

        auto result = fclose(this->handle);
        this->handle = nullptr;
        return result == 0;
    }
    auto flush() -> bool override {
        this->ensure_file_is_open();

        return fflush(this->handle) == 0;
    }
    auto read_all() -> Value override {
        // figure out how long the file is (from the current position)
        // and reset position
        long current_position = ftell(this->handle);
        fseek(this->handle, 0L, SEEK_END);
        long numbytes = ftell(this->handle) - current_position;
        fseek(this->handle, current_position, SEEK_SET);

        // allocate a buffer
        auto* buffer = new char[numbytes];

        // read the file to the buffer
        fread(buffer, sizeof(char), numbytes, this->handle);

        std::string content(buffer, numbytes);
        delete[] buffer;

        return content;
    }

    auto read_num() -> Value override {
        const size_t READ_BUFFER_SIZE = 64;

        std::string str;
        char buffer[READ_BUFFER_SIZE];

        auto current_position = ftell(this->handle);

        // consume whitespace
        fscanf(this->handle, " ");
        // collect string that could potentially be a number
        while (fscanf(this->handle, "%63[abcdefABCDEF0123456789.x-]", buffer) > 0) {
            str.append(buffer);
        }

        // try to parse it
        // if it fails to parse then we reset the file cursor and return nil
        try {
            Value number = parse_number_literal(str);
            if (number.is_nil()) {
                fseek(this->handle, current_position, SEEK_SET);
                return Nil();
            } else {
                return number;
            }
        } catch (const std::runtime_error& e) {
            fseek(this->handle, current_position, SEEK_SET);
            return Nil();
        }
    }
    auto read_line() -> Value override {
        const size_t READ_BUFFER_SIZE = 64;

        std::string line;
        char buffer[READ_BUFFER_SIZE];

        while (fgets(buffer, sizeof(buffer), this->handle) != nullptr) {
            line.append(buffer);

            // read to end of line
            if (*(line.end() - 1) == '\n') {
                line.pop_back();
                break;
            }
        }

        return line;
    }
    auto read_line_with_newline() -> Value override {
        const size_t READ_BUFFER_SIZE = 64;

        std::string line;
        char buffer[READ_BUFFER_SIZE];

        while (fgets(buffer, sizeof(buffer), this->handle) != nullptr) {
            line.append(buffer);

            // read to end of line
            if (*(line.end() - 1) == '\n') {
                break;
            }
        }

        return line;
    }
    auto read_count(long count) -> Value override {
        char buffer[count + 1];

        for (long i = 0; i < count; ++i) {
            buffer[i] = fgetc(this->handle);
            if (buffer[i] == EOF) {
                return Nil();
            }
        }

        buffer[count] = '\0';
        return std::string(buffer);
    }

    auto seek_impl(SeekWhence whence, long offset) -> long override {
        int whence_flag;
        switch (whence) {
        case SeekWhence::CURRENT:
            whence_flag = SEEK_CUR;
            break;
        case SeekWhence::SET:
            whence_flag = SEEK_SET;
            break;
        case SeekWhence::END:
            whence_flag = SEEK_END;
            break;
        default:
            throw std::runtime_error("unreachable");
        }

        if (fseek(this->handle, offset, whence_flag) == 0) {
            // success
            return ftell(this->handle);
        } else {
            // failure
            return -1;
        }
    }
    auto setvbuf_impl(SetvbufMode mode, size_t size) -> bool override {
        int mode_flag;
        switch (mode) {
        case SetvbufMode::NO:
            mode_flag = _IONBF;
            break;
        case SetvbufMode::FULL:
            mode_flag = _IOFBF;
            break;
        case SetvbufMode::LINE:
            mode_flag = _IOLBF;
            break;
        default:
            throw std::runtime_error("unreachable");
        }
        return ::setvbuf(this->handle, nullptr, mode_flag, size) == 0;
    }
    void write_string(std::string str) override {
        fwrite(str.c_str(), sizeof(char), str.size(), this->handle);
    }
};

auto static make_file_table(MemoryAllocator* allocator, CFileHandle* file) -> Value {
    Table table(allocator);

    table.set("close", [file](const CallContext& /*unused*/) -> Value { return file->close(); });
    table.set("flush", [file](const CallContext& /*unused*/) -> Value { return file->flush(); });
    table.set("write", [file](const CallContext& ctx) -> Vallist { return file->write(ctx); });
    table.set("read", [file](const CallContext& ctx) -> Vallist { return file->read(ctx); });
    table.set("seek", [file](const CallContext& ctx) -> Vallist { return file->seek(ctx); });
    table.set("lines", [file](const CallContext& ctx) -> Value { return file->lines(ctx); });
    table.set("setvbuf", [file](const CallContext& ctx) -> Value { return file->setvbuf(ctx); });
    table.set("type", [file](const CallContext& ctx) -> Value { return file->type(ctx); });

    Table metatable(allocator);
    metatable.set("__gc", [file](const CallContext& /*ctx*/) { delete file; });

    table.set_metatable(metatable);

    return table;
}

// TODO these should be taken from the environment
auto _stdin(MemoryAllocator* allocator) -> Value {
    static std::unique_ptr<CFileHandle> free_guard(new CFileHandle(stdin, false));
    return make_file_table(allocator, &*free_guard);
}
auto _stdin(const CallContext& ctx) -> Value { return _stdin(ctx.environment().allocator()); }
auto _stdout(MemoryAllocator* allocator) -> Value {
    static std::unique_ptr<CFileHandle> free_guard(new CFileHandle(stdout, false));
    return make_file_table(allocator, &*free_guard);
}
auto _stdout(const CallContext& ctx) -> Value { return _stdout(ctx.environment().allocator()); }
auto _stderr(MemoryAllocator* allocator) -> Value {
    static std::unique_ptr<CFileHandle> free_guard{new CFileHandle(stderr, false)};
    return make_file_table(allocator, &*free_guard);
}
auto _stderr(const CallContext& ctx) -> Value { return _stderr(ctx.environment().allocator()); }

static auto handle_file_open_error(const FileOpenError& error) {
    return Vallist({Nil(), error.message, error.value});
}

auto static open_file(const CallContext& ctx, const String& path, const String& mode) -> Vallist {
    try {
        auto* file = new CFileHandle(path.value, mode.value);
        Value table = make_file_table(ctx.environment().allocator(), file);
        return Vallist(table);
    } catch (const FileOpenError& error) {
        return handle_file_open_error(error);
    }
}

auto open(const CallContext& ctx) -> Vallist {
    auto file = std::get<String>(ctx.expect_argument<String>(0));
    auto mode = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [&ctx, &file](const Nil& /*unused*/) -> Vallist {
                return open_file(ctx, file, std::get<String>("r"));
            },
            [&ctx, &file](const String& mode) -> Vallist {
                std::regex modes(R"(([rwa]\+?b?))");

                if (std::regex_match(mode.value, modes)) {
                    return open_file(ctx, file, mode);
                } else {
                    throw std::runtime_error("invalid mode");
                }
            },
            [](auto /*unused*/) -> Vallist {
                throw std::runtime_error("bad argument #2 to 'open' (invalid mode)");
            }},
        mode.raw());
}

// class FileHandle
FileHandle::FileHandle() {}
FileHandle::~FileHandle() {}

void FileHandle::ensure_file_is_open() {
    if (!this->is_open()) {
        throw std::runtime_error("attempt to use a closed file");
    }
}

auto FileHandle::read(const std::vector<Value>& formats, bool throw_on_eof) -> Vallist {
    std::vector<Value> results;
    results.reserve(formats.size());

    if (formats.empty()) {
        // no format arguments provided
        return Vallist(this->read_line());
    }

    int i = 1;
    for (const auto& format : formats) {
        Value result = std::visit(
            overloaded{
                [this, i](const String& format) {
                    if (string_starts_with(format.value, 'n')) {
                        return this->read_num();
                    } else if (string_starts_with(format.value, 'a')) {
                        return this->read_all();
                    } else if (string_starts_with(format.value, 'l')) {
                        return this->read_line();
                    } else if (string_starts_with(format.value, 'L')) {
                        return this->read_line_with_newline();
                    } else {
                        throw std::runtime_error(
                            "bad argument #" + std::to_string(i) + " to 'read' (invalid format)");
                    }
                },
                [this, i](const Number& format) {
                    auto count = format.convert_to_int();

                    if (count < 0) {
                        throw std::runtime_error(
                            "bad argument #" + std::to_string(i) + " to 'read' (invalid format)");
                    } else {
                        return this->read_count(count);
                    }
                },
                [i](const auto& /*format*/) -> Value {
                    throw std::runtime_error(
                        "bad argument #" + std::to_string(i) + " to 'read' (invalid format)");
                },
            },
            format.raw());

        // if we receive nil we stop trying to read more
        // NOTE: This does not always mean that we are at the end of the file
        if (result.is_nil()) {
            if (throw_on_eof && this->is_at_eof()) {
                throw std::runtime_error("reached eof");
            }
            return Vallist(results);
        } else {
            results.push_back(result);
        }

        ++i;
    }

    return Vallist(results);
}
auto FileHandle::read(const CallContext& ctx) -> Vallist {
    this->ensure_file_is_open();

    // skip first argument because it is the file table
    auto formats_begin = ctx.arguments().begin() + 1;
    auto formats_end = ctx.arguments().end();

    std::vector<Value> formats;
    formats.reserve(std::distance(formats_begin, formats_end));
    std::copy(formats_begin, formats_end, std::back_inserter(formats));

    return this->read(formats);
}
auto FileHandle::seek(const CallContext& ctx) -> Vallist {
    const auto& whence_str = ctx.expect_argument<String>(1);
    auto offset_param = ctx.arguments().get(2);

    SeekWhence whence;
    if (whence_str == "set") {
        whence = SET;
    } else if (whence_str == "cur") {
        whence = CURRENT;
    } else if (whence_str == "end") {
        whence = END;
    } else {
        throw std::runtime_error("bad argument #1 to 'seek' (invalid option 'hi')");
    }

    long offset = std::visit(
        overloaded{
            [](const Nil& /*unused*/) -> long { return 0; },
            [](const Number& value) -> long { return value.convert_to_int(); },
            [](const auto& /*unused*/) -> long {
                throw std::runtime_error("bad argument #2 to 'seek' (invalid offset)");
            }},
        offset_param.raw());

    if (!this->is_open()) {
        return Vallist({Nil(), "Can not seek in closed file"});
    }

    try {
        return Vallist(this->seek_impl(whence, offset));
    } catch (const std::runtime_error& e) {
        return Vallist({Nil(), "Failed to seek in file"});
    }
}
auto FileHandle::write(const CallContext& ctx) -> Vallist {
    this->ensure_file_is_open();

    // arg 0 is file table
    for (size_t i = 1; i < ctx.arguments().size(); i++) {
        auto v = ctx.arguments().get(i);

        if (v.is_number() || v.is_string()) {
            String s = std::get<String>(v.to_string());
            try {
                this->write_string(s.value);
            } catch (const std::runtime_error& error) {
                return Vallist({Nil(), std::string(error.what())});
            }
        } else {
            std::string error =
                "Can't write value of type " + std::string(v.type()) + " to a file.";
            return Vallist({Nil(), error});
        }
    }

    return Vallist(ctx.arguments().get(0));
}
auto FileHandle::lines(const CallContext& ctx) -> Value {
    this->ensure_file_is_open();

    auto table = ctx.arguments().get(0);
    auto format = ctx.arguments().get(1);

    if (format.is_nil()) {
        format = "l";
    }

    return Function([this, table, format](const CallContext& ctx) {
        return this->read(ctx.make_new({table, format}));
    });
}
auto FileHandle::type(const CallContext& /*ctx*/) -> Value {
    if (this->is_open()) {
        return "file";
    } else {
        return "closed file";
    }
}
auto FileHandle::setvbuf(const CallContext& ctx) -> Value {
    if (!this->is_open()) {
        throw std::runtime_error("attempt to use a closed file");
    }

    // argument 0 is self (the file table)
    auto mode_str = std::get<String>(ctx.expect_argument<String>(1)).value;
    SetvbufMode mode;
    if (mode_str == "no") {
        mode = SetvbufMode::NO;
    } else if (mode_str == "full") {
        mode = SetvbufMode::FULL;
    } else if (mode_str == "line") {
        mode = SetvbufMode::LINE;
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'setvbuf' (invalid option '" + mode_str + "')");
    }

    size_t size = std::visit(
        overloaded{
            [](const Nil& /*unused*/) -> size_t { return BUFSIZ; },
            [](const Number& value) -> size_t { return value.convert_to_int(); },
            [](const auto& value) -> size_t {
                throw std::runtime_error(
                    "bad argument #2 to 'setvbuf' (number expected, got" + std::string(value.TYPE) +
                    ")");
            }},
        ctx.arguments().get(2).raw());

    return this->setvbuf_impl(mode, size);
}

class TmpFile : public CFileHandle {
public:
    // std::tmpfile handles creating a unique file and opening it in update mode
    // (i.e. as "wb+") and it will also automatically delete it once it's closed
    // or the program ends.
    TmpFile() : CFileHandle(std::tmpfile(), true) {}
};

auto tmpfile(const CallContext& ctx) -> Vallist {
    try {
        auto* file = new TmpFile();
        Value table = make_file_table(ctx.environment().allocator(), file);
        return Vallist(table);
    } catch (const FileOpenError& error) {
        return handle_file_open_error(error);
    }
}

struct LinesIterator {
    std::shared_ptr<CFileHandle> file;
    std::vector<Value> read_args;

    LinesIterator(std::shared_ptr<CFileHandle> file, std::vector<Value> args)
        : file(std::move(file)), read_args(std::move(args)) {}

    auto operator()(const CallContext& /*ctx*/) const -> Value {
        try {
            auto value = file->read(read_args, true).get(0);
            return value;
        } catch (const std::runtime_error&) {
            file->close();
            return Nil();
        }
    }
};

static auto __lines(const CallContext& ctx) -> Value {
    auto filename = std::get<String>(ctx.expect_argument<String>(0)).value;

    std::vector<Value> format_args;
    format_args.reserve(ctx.arguments().size() - 1);
    std::copy(ctx.arguments().begin() + 1, ctx.arguments().end(), std::back_inserter(format_args));

    auto file = std::make_shared<CFileHandle>(filename, "r");
    return Function(LinesIterator(file, format_args));
}

} // namespace io

auto create_io_table(MemoryAllocator* allocator) -> Table {
    std::unordered_map<Value, Value> math_functions;
    Table io(allocator);
    io.set("open", io::open);
    // io.set("popen", io::popen);
    io.set("tmpfile", io::tmpfile);
    io.set("type", io::type);

    io.set("stdin", io::_stdin(allocator));
    io.set("stdout", io::_stdout(allocator));
    io.set("stderr", io::_stderr(allocator));

    // used for implementing io.lines
    io.set("__lines", io::__lines);

    return io;
}

} // end namespace minilua
