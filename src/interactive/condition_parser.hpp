#pragma once

#include "interactive/metadata_builder.hpp"
#include "metadata/schema.hpp"
#include <memory>
#include <string>

namespace scaffolder {

/** Parse a Jira-style condition expression into ConditionData.
 * Syntax: var equals value | var in (a, b) | var not_in (a, b)
 *        expr AND expr | expr OR expr | NOT expr | ( expr ) | default
 * Returns nullptr on parse error; error message in out_error if non-null. */
std::shared_ptr<ConditionData> parse_condition_text(const std::string& text, std::string* out_error = nullptr);

/** Serialize ConditionData to the same text syntax (for editing). */
std::string condition_to_text(const std::shared_ptr<ConditionData>& cond);

/** Convert ConditionData (parser output) to schema Condition for JSON serialization.
 * Compound conditions (AND/OR/NOT) become "and"/"or"/"not" in the emitted JSON. */
Condition condition_data_to_schema(const std::shared_ptr<ConditionData>& cd);

}  // namespace scaffolder
