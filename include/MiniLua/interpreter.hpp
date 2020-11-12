#include <memory>
#include <string>
#include <vector>

#include "environment.hpp"
#include "source_change.hpp"
#include "values.hpp"

namespace minilua {

struct ParseResult {
    std::vector<std::string> errors;

    operator bool() const;
};

struct EvalResult {
    Value value;
    std::vector<SourceChange> source_changes;
};

class Interpreter {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    Interpreter();
    Interpreter(std::string initial_source_code);
    ~Interpreter();

    /**
     * Returns the environment for modification.
     */
    [[nodiscard]] auto environment() const -> Environment&;

    /**
     * Returns a view into the current source code.
     *
     * The returned value will become invalid if the source code is changed
     * (by calling parse or apply_source_changes).
     */
    [[nodiscard]] auto source_code() const -> std::string_view;

    /**
     * Parse fresh source code.
     *
     * Throws an exception if there was an error parsing the source code.
     */
    auto parse(std::string source_code) -> ParseResult;

    /**
     * Applies a source change.
     *
     * A source change can be a bigger tree of and-ed and or-ed changes. For
     * or-ed changes only the first branch of the tree will be applied.
     */
    void apply_source_change(SourceChange);

    /**
     * Run the parsed program.
     *
     * TODO should the user be able to provide parameters?
     * - not sure how you would provide them to lua
     */
    auto evaluate() -> EvalResult;
};

}; // namespace minilua
