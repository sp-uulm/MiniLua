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

#include "MiniLua/environment.hpp"
#include "MiniLua/io.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua {

auto create_io_table(MemoryAllocator* allocator) -> Table {
    std::unordered_map<Value, Value> math_functions;
    Table io(allocator);
    io.set("open", io::open);
    io.set("close", io::close);
    // io.set("flush", io::flush);
    // io.set("input", io::input);
    // io.set("lines", io::lines);
    // io.set("output", io::output);
    // io.set("popen", io::popen);
    // io.set("read", io::read);
    // io.set("tmpfile", io::tmpfile);
    io.set("type", io::type);
    // io.set("write", io::write);

    // io.set("stdin", Nil());
    // io.set("stdout", Nil());
    // io.set("stderr", Nil());
    return io;
}

namespace io {

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

class FileHandle {
public:
    enum SeekWhence { SET, CURRENT, END };

    FileHandle() {}
    virtual ~FileHandle(){};

    auto read(const CallContext& ctx) -> Vallist {
        if (!this->is_open()) {
            // TODO error?
            return Vallist();
        }
        // auto file_table = ctx.arguments().get(0);
        auto format = ctx.arguments().get(1);

        if (format != "a") {
            throw std::runtime_error("Currently only 'a' is implemented for formats.");
        }

        // TODO allow the other and multiple formats

        return Vallist(this->read_all());
    }

    auto seek(const CallContext& ctx) -> Vallist {
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

    auto write(const CallContext& ctx) -> Vallist {
        if (!this->is_open()) {
            // TODO error?
            return Vallist();
        }

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
        if (!this->is_open()) {
            // TODO error?
            return Nil();
        }

        // TODO
        // create iterator function that call read with the given format
        return Nil();
    }

    auto type(const CallContext& ctx) -> Value {
        if (this->is_open()) {
            return "file";
        } else {
            return "closed file";
        }
    }

    virtual auto is_open() -> bool = 0;

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
    // NOTE: a nullptr is treated as a closed file
    FILE* handle;

public:
    CFileHandle(const std::string& path, const std::string& mode) {
        this->handle = std::fopen(path.c_str(), mode.c_str());
    }
    ~CFileHandle() override { this->close(); }

    auto is_open() -> bool override { return this->handle != nullptr; }

    auto close() -> bool override {
        if (!is_open()) {
            // TODO error?
            return false;
        }
        auto result = fclose(this->handle);
        this->handle = nullptr;
        return result == 0;
    }
    auto flush() -> bool override {
        if (!is_open()) {
            // TODO error?
            return false;
        }
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
        }

        if (fseek(this->handle, offset, whence_flag) == 0) {
            // success
            return ftell(this->handle);
        } else {
            // failure
            return -1;
        }
    }
    auto setvbuf(const CallContext& ctx) -> Value override {
        if (!is_open()) {
            // TODO error?
            return Nil();
        }
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

auto static open_file(const CallContext& ctx, const String& path, const String& mode) -> Vallist {
    try {
        auto* file = new CFileHandle(path.value, mode.value);

        Table table = ctx.make_table();

        table.set(
            "close", [file](const CallContext& /*unused*/) -> Value { return file->close(); });
        table.set(
            "flush", [file](const CallContext& /*unused*/) -> Value { return file->flush(); });
        table.set("write", [file](const CallContext& ctx) -> Vallist { return file->write(ctx); });
        table.set("read", [file](const CallContext& ctx) -> Vallist { return file->read(ctx); });
        table.set("seek", [file](const CallContext& ctx) -> Vallist { return file->seek(ctx); });
        table.set("lines", [file](const CallContext& ctx) -> Value { return file->lines(ctx); });
        table.set(
            "setvbuf", [file](const CallContext& ctx) -> Value { return file->setvbuf(ctx); });
        table.set("type", [file](const CallContext& ctx) -> Value { return file->type(ctx); });

        Table metatable = ctx.make_table();
        metatable.set("__gc", [file](const CallContext& /*ctx*/) { delete file; });

        table.set_metatable(metatable);

        return Vallist(table);
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

auto close(const CallContext& ctx) -> CallResult {
    auto file = ctx.expect_argument<Table>(0);

    return file["close"].call(ctx.make_new());
}

} // namespace io
} // end namespace minilua
