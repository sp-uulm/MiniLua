#include <memory>
#include <string>
#include <vector>

#include "environment.hpp"
#include "source_change.hpp"

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

struct EvalResult {
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
    [[nodiscard]] auto environment() -> Environment&;

    /**
     * Parse fresh source code.
     */
    void parse(std::string source_code);

    /**
     * Applies source changes.
     *
     * These can be created inside or outside the interpreter.
     */
    void apply_source_changes(std::vector<SourceChange>);

    /**
     * Run the parsed program.
     *
     * TODO should the user be able to return a result from lua?
     * - we would need to make the actual syntax tree part of the public interface
     * - OR we need to serialize it in some way (e.g. json)
     *
     * TODO should the user be able to provide parameters?
     * - not sure how you would provide them to lua
     */
    auto run() -> EvalResult;
};

}; // namespace minilua
