#include <memory>
#include <string>
#include <vector>

#include "environment.hpp"
#include "source_change.hpp"
#include "values.hpp"

namespace minilua {

struct SuggestedSourceChange {
    // can be filled in by the function creating the suggestion
    std::optional<std::string> origin;
    // hint for the source locations that would be modified
    // TODO maybe should be part of the SourceChange
    std::string hint;
    // TODO maybe this needs to be a vector
    SourceChange change;
};

struct ParseResult {
    std::vector<std::string> errors;

    operator bool() const;
};

struct EvalResult {
    Value value;
    std::vector<SuggestedSourceChange> source_change_suggestions;
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
     * Applies source changes.
     *
     * These can be created inside or outside the interpreter.
     */
    void apply_source_changes(std::vector<SourceChange>);

    /**
     * Run the parsed program.
     *
     * TODO should the user be able to provide parameters?
     * - not sure how you would provide them to lua
     */
    auto run() -> EvalResult;
};

}; // namespace minilua
