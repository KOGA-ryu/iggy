#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

#include <charconv>
#include <cctype>
#include <string_view>

namespace iggy::runtime {
namespace {

enum class Table {
	Root,
	Grid,
	NoClaims,
	Promotion,
	Legend,
};

bool IsWhitespaceOnly(const std::string &text)
{
	for (const unsigned char character : text) {
		if (!std::isspace(character)) {
			return false;
		}
	}
	return true;
}

std::string Trim(std::string_view value)
{
	std::size_t first = 0;
	while (first < value.size() &&
		std::isspace(static_cast<unsigned char>(value[first]))) {
		++first;
	}

	std::size_t last = value.size();
	while (last > first &&
		std::isspace(static_cast<unsigned char>(value[last - 1]))) {
		--last;
	}

	return std::string(value.substr(first, last - first));
}

std::string StripComment(std::string_view line)
{
	bool inString = false;
	bool escaped = false;
	for (std::size_t index = 0; index < line.size(); ++index) {
		const char character = line[index];
		if (escaped) {
			escaped = false;
			continue;
		}
		if (inString && character == '\\') {
			escaped = true;
			continue;
		}
		if (character == '"') {
			inString = !inString;
			continue;
		}
		if (!inString && character == '#') {
			return std::string(line.substr(0, index));
		}
	}
	return std::string(line);
}

std::vector<std::string> Lines(const std::string &text)
{
	std::vector<std::string> result;
	std::size_t start = 0;
	while (start <= text.size()) {
		const std::size_t end = text.find('\n', start);
		if (end == std::string::npos) {
			result.push_back(text.substr(start));
			break;
		}
		result.push_back(text.substr(start, end - start));
		start = end + 1;
	}
	return result;
}

void AddIssue(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError:
		++result.syntaxIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType:
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString:
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnknownEnumValue:
		++result.typeIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape:
		++result.unsupportedIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid:
		++result.sourcePlanIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::MissingTable:
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::MissingRequiredKey:
		++result.syntaxIssueCount;
		break;
	}

	result.issues.push_back(issue);
	result.issueCount = result.issues.size();
}

bool ParseQuotedString(
	const std::string &value,
	std::string &out)
{
	if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
		return false;
	}

	out.clear();
	bool escaped = false;
	for (std::size_t index = 1; index + 1 < value.size(); ++index) {
		const char character = value[index];
		if (escaped) {
			switch (character) {
			case '"':
			case '\\':
				out.push_back(character);
				break;
			case 'n':
				out.push_back('\n');
				break;
			case 't':
				out.push_back('\t');
				break;
			default:
				return false;
			}
			escaped = false;
			continue;
		}
		if (character == '\\') {
			escaped = true;
			continue;
		}
		out.push_back(character);
	}
	return !escaped;
}

bool ParseUnsigned(
	const std::string &value,
	std::size_t &out)
{
	if (value.empty() || value.front() == '-') {
		return false;
	}
	std::size_t parsed = 0;
	const char *begin = value.data();
	const char *end = value.data() + value.size();
	const std::from_chars_result result = std::from_chars(begin, end, parsed);
	if (result.ec != std::errc {} || result.ptr != end) {
		return false;
	}
	out = parsed;
	return true;
}

bool ParseBool(
	const std::string &value,
	bool &out)
{
	if (value == "true") {
		out = true;
		return true;
	}
	if (value == "false") {
		out = false;
		return true;
	}
	return false;
}

std::vector<ResourceId> ToResourceIds(const std::vector<std::string> &values)
{
	std::vector<ResourceId> result;
	result.reserve(values.size());
	for (const std::string &value : values) {
		result.push_back(ResourceId(value));
	}
	return result;
}

bool ParseGlyph(
	const std::string &value,
	char &out)
{
	std::string parsed;
	if (!ParseQuotedString(value, parsed) || parsed.size() != 1 ||
		static_cast<unsigned char>(parsed[0]) > 0x7F) {
		return false;
	}
	out = parsed[0];
	return true;
}

bool ParseGlyphKind(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanGlyphKind &out)
{
	if (value == "background") {
		out = RuntimeGameplayAsciiSourcePlanGlyphKind::Background;
		return true;
	}
	if (value == "terrain") {
		out = RuntimeGameplayAsciiSourcePlanGlyphKind::Terrain;
		return true;
	}
	if (value == "actor") {
		out = RuntimeGameplayAsciiSourcePlanGlyphKind::Actor;
		return true;
	}
	if (value == "player_start") {
		out = RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart;
		return true;
	}
	if (value == "region_marker") {
		out = RuntimeGameplayAsciiSourcePlanGlyphKind::RegionMarker;
		return true;
	}
	if (value == "annotation") {
		out = RuntimeGameplayAsciiSourcePlanGlyphKind::Annotation;
		return true;
	}
	if (value == "unknown") {
		out = RuntimeGameplayAsciiSourcePlanGlyphKind::Unknown;
		return true;
	}
	return false;
}

bool ParseMarkerKind(
	const std::string &value,
	RuntimeGameplayAsciiScenarioMarkerKind &out)
{
	if (value == "empty") {
		out = RuntimeGameplayAsciiScenarioMarkerKind::Empty;
		return true;
	}
	if (value == "floor") {
		out = RuntimeGameplayAsciiScenarioMarkerKind::Floor;
		return true;
	}
	if (value == "wall") {
		out = RuntimeGameplayAsciiScenarioMarkerKind::Wall;
		return true;
	}
	if (value == "actor") {
		out = RuntimeGameplayAsciiScenarioMarkerKind::Actor;
		return true;
	}
	if (value == "player_start") {
		out = RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart;
		return true;
	}
	if (value == "unknown") {
		out = RuntimeGameplayAsciiScenarioMarkerKind::Unknown;
		return true;
	}
	return false;
}

bool ParseStringArrayInline(
	const std::string &value,
	std::vector<std::string> &out)
{
	if (value.size() < 2 || value.front() != '[' || value.back() != ']') {
		return false;
	}
	const std::string body = Trim(std::string_view(value).substr(1, value.size() - 2));
	out.clear();
	if (body.empty()) {
		return true;
	}

	std::size_t index = 0;
	while (index < body.size()) {
		while (index < body.size() &&
			std::isspace(static_cast<unsigned char>(body[index]))) {
			++index;
		}
		if (index >= body.size() || body[index] != '"') {
			return false;
		}
		std::size_t end = index + 1;
		bool escaped = false;
		for (; end < body.size(); ++end) {
			const char character = body[end];
			if (escaped) {
				escaped = false;
				continue;
			}
			if (character == '\\') {
				escaped = true;
				continue;
			}
			if (character == '"') {
				break;
			}
		}
		if (end >= body.size()) {
			return false;
		}
		std::string parsed;
		if (!ParseQuotedString(body.substr(index, end - index + 1), parsed)) {
			return false;
		}
		out.push_back(parsed);
		index = end + 1;
		while (index < body.size() &&
			std::isspace(static_cast<unsigned char>(body[index]))) {
			++index;
		}
		if (index == body.size()) {
			return true;
		}
		if (body[index] != ',') {
			return false;
		}
		++index;
	}
	return true;
}

bool ParseMultilineStringArrayItem(
	const std::string &value,
	std::string &out,
	bool &closed)
{
	closed = value == "]";
	if (closed) {
		return true;
	}

	std::string item = value;
	if (!item.empty() && item.back() == ',') {
		item.pop_back();
		item = Trim(item);
	}
	return ParseQuotedString(item, out);
}

void AddWrongType(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &key)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType;
	issue.line = line;
	issue.key = key;
	AddIssue(result, issue);
}

void AddInvalidGlyph(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &key)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString;
	issue.line = line;
	issue.key = key;
	AddIssue(result, issue);
}

void AddUnknownEnum(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &key,
	const std::string &value)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnknownEnumValue;
	issue.line = line;
	issue.key = key;
	issue.detail = value;
	AddIssue(result, issue);
}

void AddSyntax(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &detail)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError;
	issue.line = line;
	issue.detail = detail;
	AddIssue(result, issue);
}

void AddUnsupported(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &detail)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape;
	issue.line = line;
	issue.detail = detail;
	AddIssue(result, issue);
}

void AddMissingTable(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	const std::string &table)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::MissingTable;
	issue.key = table;
	AddIssue(result, issue);
}

RuntimeGameplayAsciiSourcePlanTomlReadStatus StatusForParserIssues(
	const RuntimeGameplayAsciiSourcePlanTomlReadResult &result)
{
	if (result.unsupportedIssueCount > 0) {
		return RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax;
	}
	if (result.typeIssueCount > 0) {
		return RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid;
	}
	return RuntimeGameplayAsciiSourcePlanTomlReadStatus::SyntaxInvalid;
}

} // namespace

bool RuntimeGameplayAsciiSourcePlanTomlReadResult::ok() const
{
	return status == RuntimeGameplayAsciiSourcePlanTomlReadStatus::Parsed;
}

RuntimeGameplayAsciiSourcePlanTomlReadResult RuntimeGameplayAsciiSourcePlanTomlReader::read(
	const std::string &text) const
{
	RuntimeGameplayAsciiSourcePlanTomlReadResult result;
	result.input = text;
	result.inputSize = text.size();

	if (IsWhitespaceOnly(text)) {
		RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError;
		issue.detail = "empty TOML source-plan input";
		AddIssue(result, issue);
		result.status = RuntimeGameplayAsciiSourcePlanTomlReadStatus::SyntaxInvalid;
		return result;
	}

	Table table = Table::Root;
	bool sawGrid = false;
	bool readingRows = false;
	std::vector<std::string> parsedRows;

	const std::vector<std::string> lines = Lines(text);
	for (std::size_t index = 0; index < lines.size(); ++index) {
		const std::size_t lineNumber = index + 1;
		const std::string line = Trim(StripComment(lines[index]));
		if (line.empty()) {
			continue;
		}

		if (readingRows) {
			std::string row;
			bool closed = false;
			if (!ParseMultilineStringArrayItem(line, row, closed)) {
				AddWrongType(result, lineNumber, "rows");
				continue;
			}
			if (closed) {
				result.plan.grid.rows = parsedRows;
				readingRows = false;
			} else {
				parsedRows.push_back(row);
			}
			continue;
		}

		if (line == "[grid]") {
			table = Table::Grid;
			sawGrid = true;
			continue;
		}
		if (line == "[no_claims]") {
			table = Table::NoClaims;
			continue;
		}
		if (line == "[promotion]") {
			table = Table::Promotion;
			continue;
		}
		if (line == "[[legend]]") {
			result.plan.legend.push_back({});
			table = Table::Legend;
			continue;
		}
		if (!line.empty() && line.front() == '[') {
			AddUnsupported(result, lineNumber, "unsupported TOML table in source-plan reader slice");
			continue;
		}

		const std::size_t equals = line.find('=');
		if (equals == std::string::npos) {
			AddSyntax(result, lineNumber, "expected key = value");
			continue;
		}
		const std::string key = Trim(std::string_view(line).substr(0, equals));
		const std::string value = Trim(std::string_view(line).substr(equals + 1));
		if (key.empty()) {
			AddSyntax(result, lineNumber, "empty key");
			continue;
		}

		if (table == Table::Root) {
			if (key == "format_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					result.plan.formatId = ResourceId(parsed);
				}
			} else if (key == "version") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					result.plan.version = parsed;
				}
			} else if (key == "source_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					result.plan.hasSourceId = true;
					result.plan.sourceId = ResourceId(parsed);
				}
			} else if (key == "source_ref") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					result.plan.hasSourceRef = true;
					result.plan.sourceRef = ResourceId(parsed);
				}
			} else {
				AddUnsupported(result, lineNumber, "unsupported root key: " + key);
			}
			continue;
		}

		if (table == Table::NoClaims) {
			bool parsed = false;
			if (!ParseBool(value, parsed)) {
				AddWrongType(result, lineNumber, key);
			} else if (key == "runtime_truth") {
				result.plan.noClaims.claimsRuntimeTruth = parsed;
			} else if (key == "gameplay_execution") {
				result.plan.noClaims.claimsGameplayExecution = parsed;
			} else if (key == "file_parsing") {
				result.plan.noClaims.claimsFileParsing = parsed;
			} else if (key == "profile_scenario_conversion") {
				result.plan.noClaims.claimsProfileScenarioConversion = parsed;
			} else {
				AddUnsupported(result, lineNumber, "unsupported no_claims key: " + key);
			}
			continue;
		}

		if (table == Table::Promotion) {
			bool parsed = false;
			if (!ParseBool(value, parsed)) {
				AddWrongType(result, lineNumber, key);
			} else if (key == "ready") {
				result.plan.promotionPolicy.promotionReady = parsed;
			} else if (key == "runtime_execution") {
				result.plan.promotionPolicy.allowsRuntimeExecution = parsed;
			} else if (key == "file_parsing") {
				result.plan.promotionPolicy.allowsFileParsing = parsed;
			} else if (key == "profile_scenario_conversion") {
				result.plan.promotionPolicy.allowsProfileScenarioConversion = parsed;
			} else {
				AddUnsupported(result, lineNumber, "unsupported promotion key: " + key);
			}
			continue;
		}

		if (table == Table::Legend) {
			RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry =
				result.plan.legend.back();
			if (key == "glyph") {
				char glyph = '\0';
				if (!ParseGlyph(value, glyph)) {
					AddInvalidGlyph(result, lineNumber, key);
				} else {
					entry.glyph = glyph;
				}
			} else if (key == "kind") {
				std::string parsed;
				RuntimeGameplayAsciiSourcePlanGlyphKind kind =
					RuntimeGameplayAsciiSourcePlanGlyphKind::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else if (!ParseGlyphKind(parsed, kind)) {
					AddUnknownEnum(result, lineNumber, key, parsed);
				} else {
					entry.kind = kind;
				}
			} else if (key == "role_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					entry.roleId = ResourceId(parsed);
				}
			} else if (key == "role_tags") {
				std::vector<std::string> parsed;
				if (!ParseStringArrayInline(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					entry.roleTags = ToResourceIds(parsed);
				}
			} else if (key == "maps_to_scenario_marker") {
				bool parsed = false;
				if (!ParseBool(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					entry.mapsToScenarioMarker = parsed;
				}
			} else if (key == "scenario_marker_kind") {
				std::string parsed;
				RuntimeGameplayAsciiScenarioMarkerKind kind =
					RuntimeGameplayAsciiScenarioMarkerKind::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else if (!ParseMarkerKind(parsed, kind)) {
					AddUnknownEnum(result, lineNumber, key, parsed);
				} else {
					entry.scenarioMarkerKind = kind;
				}
			} else if (key == "target_marker_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					entry.targetMarkerId = ResourceId(parsed);
				}
			} else if (key == "target_profile_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					entry.targetProfileId = ResourceId(parsed);
				}
			} else {
				AddUnsupported(result, lineNumber, "unsupported legend key: " + key);
			}
			continue;
		}

		if (table == Table::Grid) {
			if (key == "width") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					result.plan.grid.width = parsed;
				}
			} else if (key == "height") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key);
				} else {
					result.plan.grid.height = parsed;
				}
			} else if (key == "background") {
				char glyph = '\0';
				if (!ParseGlyph(value, glyph)) {
					AddInvalidGlyph(result, lineNumber, key);
				} else {
					result.plan.grid.backgroundGlyph = glyph;
				}
			} else if (key == "rows") {
				if (value == "[") {
					readingRows = true;
					parsedRows.clear();
					continue;
				}
				std::vector<std::string> rows;
				if (!ParseStringArrayInline(value, rows)) {
					AddWrongType(result, lineNumber, key);
				} else {
					result.plan.grid.rows = rows;
				}
			} else {
				AddUnsupported(result, lineNumber, "unsupported grid key: " + key);
			}
		}
	}

	if (readingRows) {
		AddSyntax(result, lines.size(), "unterminated rows array");
	}

	if (!sawGrid) {
		AddMissingTable(result, "grid");
	}

	if (!result.issues.empty()) {
		result.status = StatusForParserIssues(result);
		return result;
	}

	result.sourceValidation = RuntimeGameplayAsciiSourcePlanValidator {}.validate(result.plan);
	result.sourcePlanIssueCount = result.sourceValidation.issueCount;
	if (!result.sourceValidation.ok()) {
		RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid;
		issue.detail = "parsed TOML source plan failed source-plan validation";
		AddIssue(result, issue);
		result.status = RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid;
		return result;
	}

	result.status = RuntimeGameplayAsciiSourcePlanTomlReadStatus::Parsed;
	return result;
}

} // namespace iggy::runtime
