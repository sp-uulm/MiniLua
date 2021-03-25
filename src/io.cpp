#include <bits/types/FILE.h>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <istream>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "MiniLua/io.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua {

auto create_io_table(MemoryAllocator* allocator) -> Table {
    std::unordered_map<Value, Value> math_functions;
    Table io(allocator);
    io.set("open", io::open);
    // io.set("close", io::close);
    // io.set("flush", io::flush);
    // io.set("input", io::input);
    // io.set("lines", io::lines);
    // io.set("output", io::output);
    // io.set("popen", io::popen);
    // io.set("read", io::read);
    // io.set("tmpfile", io::tmpfile);
    // io.set("type", io::type);
    // io.set("write", io::write);

    // io.set("stdin", Nil());
    // io.set("stdout", Nil());
    // io.set("stderr", Nil());
    return io;
}

namespace io {

class FileHandle {
protected:
    bool closed;

public:
    enum SeekWhence { SET, CURRENT, END };

    FileHandle() : closed(false) {}
    virtual ~FileHandle(){};

    auto read(const CallContext& ctx) -> Vallist {
        // auto file_table = ctx.arguments().get(0);
        auto format = ctx.arguments().get(1);

        if (format != "a") {
            throw std::runtime_error("Currently only 'a' is implemented for formats.");
        }

        // TODO allow the other and multuple formats

        return Vallist(this->read_all());
    }

    auto seek(const CallContext& ctx) -> Value {
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

        return this->seek_impl(whence, offset);
    }

    auto write(const CallContext& ctx) -> Vallist {
        // arg 0 is file table
        for (int i = 1; i < ctx.arguments().size(); i++) {
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

    auto lines(const CallContext& ctx) -> Value {
        // TODO
        // create iterator function that call read with the given format
        return Nil();
    }

    virtual auto close() -> bool = 0;
    virtual auto flush() -> bool = 0;
    virtual auto seek_impl(SeekWhence whence, long offset) -> long = 0;

    virtual auto read_all() -> Value = 0;
    // virtual auto read_num() -> Value;
    // virtual auto read_line() -> Value;
    // virtual auto read_line_with_newline() -> Value;
    // virtual auto read_count() -> Value;

    // file:seek([whence [, offset]]])
    // file:setvbuf(mode, [, size])
    virtual auto setvbuf(const CallContext& ctx) -> Value = 0;
    virtual void write_string(std::string str) = 0;
};

class CFileHandle : public FileHandle {
    FILE* handle;

public:
    CFileHandle(const std::string& path, const std::string& mode) {
        this->handle = std::fopen(path.c_str(), mode.c_str());
    }
    ~CFileHandle() override { this->close(); }

    auto close() -> bool override { return fclose(this->handle) == 0; }
    auto flush() -> bool override { return fflush(this->handle) == 0; }
    auto read_all() -> Value override {
        // figure out how long the file is (and reset to start of file)
        fseek(this->handle, 0L, SEEK_END);
        long numbytes = ftell(this->handle);
        fseek(this->handle, 0L, SEEK_SET);

        // allocate a buffer
        auto* buffer = new char[numbytes];

        // read the file to the buffer
        fread(buffer, sizeof(char), numbytes, this->handle);

        std::string content(buffer, numbytes);
        delete[] buffer;

        return content;
    }
    auto seek_impl(SeekWhence whence, long offset) -> long override {
        // TODO
        return 0;
    }
    auto setvbuf(const CallContext& ctx) -> Value override {
        // do nothing
        return true;
    }
    void write_string(std::string str) override {
        fwrite(str.c_str(), sizeof(char), str.size(), this->handle);
    }
};

// TODO FileHandle for C++ streams so we can use stdin, stdout and stderr

std::istream* stdin = &std::cin;
std::ostream* stdout = &std::cout;
std::ostream* stderr = &std::cerr;

auto static open_file(const String& path, const String& mode) -> Vallist {
    try {
        auto* file = new CFileHandle(path.value, mode.value);

        Table file_tab;

        file_tab.set(
            "close", [file](const CallContext& /*unused*/) -> Value { return file->close(); });
        file_tab.set(
            "flush", [file](const CallContext& /*unused*/) -> Value { return file->flush(); });
        file_tab.set(
            "write", [file](const CallContext& ctx) -> Vallist { return file->write(ctx); });
        file_tab.set("read", [file](const CallContext& ctx) -> Vallist { return file->read(ctx); });
        file_tab.set("seek", [file](const CallContext& ctx) -> Value { return file->seek(ctx); });
        file_tab.set("lines", [file](const CallContext& ctx) -> Value { return file->lines(ctx); });
        file_tab.set(
            "setvbuf", [file](const CallContext& ctx) -> Value { return file->setvbuf(ctx); });

        return Vallist(file_tab);
    } catch (const std::runtime_error& error) {
        // TODO: differnciate between different errors
        return Vallist({Nil(), "could not open file", 2});
    }
}

auto open(const CallContext& ctx) -> Vallist {
    auto file = std::get<String>(ctx.expect_argument<String>(0));
    auto mode = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [&file](const Nil& /*unused*/) -> Vallist {
                return open_file(file, std::get<String>("r"));
            },
            [&file](const String& mode) -> Vallist {
                std::regex modes(R"(([rwa]\+?b?))");

                if (std::regex_match(mode.value, modes)) {
                    return open_file(file, mode);
                } else {
                    throw std::runtime_error("invalid mode");
                }
            },
            [](auto /*unused*/) -> Vallist {
                throw std::runtime_error("bad argument #2 to 'open' (invalid mode)");
            }},
        mode.raw());
}
} // namespace io
} // end namespace minilua
