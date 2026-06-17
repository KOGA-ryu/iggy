#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

#include <charconv>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <string_view>
#include <utility>

namespace iggy::runtime {
namespace {

enum class Table {
	Root,
	Grid,
	NoClaims,
	Promotion,
	Expect,
	ExpectTraceFrames,
	ExpectInventoryStacks,
	Legend,
	Cells,
	Regions,
	Profiles,
	InteractionTargets,
	ItemDrops,
	FrameControls,
	FramePlayerCommands,
};

enum class InlineShapeStatus {
	Ok,
	WrongType,
	Unsupported,
};

enum class MultilineStringArrayTarget {
	None,
	GridRows,
	ExpectFinalRows,
	ExpectTraceFrameRows,
};

struct IssueContext {
	Table table = Table::Root;
	bool hasTableIndex = false;
	std::size_t tableIndex = 0;
};

std::string TableName(Table table)
{
	switch (table) {
	case Table::Root:
		return "root";
	case Table::Grid:
		return "grid";
	case Table::NoClaims:
		return "no_claims";
	case Table::Promotion:
		return "promotion";
	case Table::Expect:
		return "expect";
	case Table::ExpectTraceFrames:
		return "expect_trace_frames";
	case Table::ExpectInventoryStacks:
		return "expect_inventory_stacks";
	case Table::Legend:
		return "legend";
	case Table::Cells:
		return "cells";
	case Table::Regions:
		return "regions";
	case Table::Profiles:
		return "profiles";
	case Table::InteractionTargets:
		return "interaction_targets";
	case Table::ItemDrops:
		return "item_drops";
	case Table::FrameControls:
		return "frame_controls";
	case Table::FramePlayerCommands:
		return "frame_player_commands";
	}
	return {};
}

void ApplyContext(
	RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue,
	const IssueContext &context)
{
	issue.table = TableName(context.table);
	issue.hasTableIndex = context.hasTableIndex;
	issue.tableIndex = context.tableIndex;
}

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

bool ParseSigned(
	const std::string &value,
	int &out)
{
	if (value.empty()) {
		return false;
	}
	int parsed = 0;
	const char *begin = value.data();
	const char *end = value.data() + value.size();
	const std::from_chars_result result = std::from_chars(begin, end, parsed);
	if (result.ec != std::errc {} || result.ptr != end) {
		return false;
	}
	out = parsed;
	return true;
}

bool ParseDouble(
	const std::string &value,
	double &out)
{
	if (value.empty()) {
		return false;
	}
	errno = 0;
	char *end = nullptr;
	const double parsed = std::strtod(value.c_str(), &end);
	if (errno != 0 || end == value.c_str() || *end != '\0') {
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

bool ParseControlBehavior(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanControlBehavior &out)
{
	if (value == "waiting") {
		out = RuntimeGameplayAsciiSourcePlanControlBehavior::Waiting;
		return true;
	}
	if (value == "seeking") {
		out = RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking;
		return true;
	}
	return false;
}

bool ParseControlMoveMode(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanControlMoveMode &out)
{
	if (value == "still") {
		out = RuntimeGameplayAsciiSourcePlanControlMoveMode::Still;
		return true;
	}
	if (value == "walk") {
		out = RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk;
		return true;
	}
	if (value == "jog") {
		out = RuntimeGameplayAsciiSourcePlanControlMoveMode::Jog;
		return true;
	}
	if (value == "run") {
		out = RuntimeGameplayAsciiSourcePlanControlMoveMode::Run;
		return true;
	}
	if (value == "sprint") {
		out = RuntimeGameplayAsciiSourcePlanControlMoveMode::Sprint;
		return true;
	}
	return false;
}

bool ParsePlayerCommandKind(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanPlayerCommandKind &out)
{
	if (value == "move_to_tile") {
		out = RuntimeGameplayAsciiSourcePlanPlayerCommandKind::MoveToTile;
		return true;
	}
	if (value == "interact") {
		out = RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact;
		return true;
	}
	if (value == "pickup") {
		out = RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup;
		return true;
	}
	return false;
}

bool ParseInteractionTargetKind(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanInteractionTargetKind &out)
{
	if (value == "inspectable") {
		out = RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Inspectable;
		return true;
	}
	if (value == "usable") {
		out = RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Usable;
		return true;
	}
	if (value == "pickup") {
		out = RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Pickup;
		return true;
	}
	if (value == "talk") {
		out = RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Talk;
		return true;
	}
	if (value == "door") {
		out = RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Door;
		return true;
	}
	return false;
}

bool ParseInteractionEffectKind(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanInteractionEffectKind &out)
{
	if (value == "none") {
		out = RuntimeGameplayAsciiSourcePlanInteractionEffectKind::None;
		return true;
	}
	if (value == "inspect_text") {
		out = RuntimeGameplayAsciiSourcePlanInteractionEffectKind::InspectText;
		return true;
	}
	if (value == "toggle_target") {
		out = RuntimeGameplayAsciiSourcePlanInteractionEffectKind::ToggleTarget;
		return true;
	}
	if (value == "emit_event") {
		out = RuntimeGameplayAsciiSourcePlanInteractionEffectKind::EmitEvent;
		return true;
	}
	if (value == "pickup_item") {
		out = RuntimeGameplayAsciiSourcePlanInteractionEffectKind::PickupItem;
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

bool ParseInlineTableFields(
	const std::string &value,
	std::vector<std::pair<std::string, std::string>> &out)
{
	if (value.size() < 2 || value.front() != '{' || value.back() != '}') {
		return false;
	}

	const std::string body = Trim(std::string_view(value).substr(1, value.size() - 2));
	out.clear();
	if (body.empty()) {
		return true;
	}

	std::size_t start = 0;
	while (start < body.size()) {
		bool inString = false;
		bool escaped = false;
		std::size_t end = start;
		for (; end < body.size(); ++end) {
			const char character = body[end];
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
			if (!inString && character == ',') {
				break;
			}
		}
		if (inString || escaped) {
			return false;
		}

		const std::string item = Trim(std::string_view(body).substr(start, end - start));
		const std::size_t equals = item.find('=');
		if (item.empty() || equals == std::string::npos) {
			return false;
		}
		const std::string key = Trim(std::string_view(item).substr(0, equals));
		const std::string fieldValue = Trim(std::string_view(item).substr(equals + 1));
		if (key.empty() || fieldValue.empty()) {
			return false;
		}
		out.push_back({ key, fieldValue });
		start = end + 1;
	}

	return true;
}

bool FindInlineField(
	const std::vector<std::pair<std::string, std::string>> &fields,
	const std::string &key,
	std::string &out)
{
	for (const auto &field : fields) {
		if (field.first == key) {
			out = field.second;
			return true;
		}
	}
	return false;
}

bool InlineFieldsOnlyContain(
	const std::vector<std::pair<std::string, std::string>> &fields,
	std::initializer_list<const char *> keys)
{
	for (const auto &field : fields) {
		bool found = false;
		for (const char *key : keys) {
			if (field.first == key) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}

InlineShapeStatus ParseLocalTile(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanLocalTile &out)
{
	std::vector<std::pair<std::string, std::string>> fields;
	if (!ParseInlineTableFields(value, fields) ||
		!InlineFieldsOnlyContain(fields, { "x", "y" })) {
		return InlineShapeStatus::Unsupported;
	}

	std::string xValue;
	std::string yValue;
	int x = 0;
	int y = 0;
	if (!FindInlineField(fields, "x", xValue) ||
		!FindInlineField(fields, "y", yValue)) {
		return InlineShapeStatus::Unsupported;
	}
	if (!ParseSigned(xValue, x) || !ParseSigned(yValue, y)) {
		return InlineShapeStatus::WrongType;
	}

	out.present = true;
	out.x = x;
	out.y = y;
	return InlineShapeStatus::Ok;
}

InlineShapeStatus ParseLocalPosition(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanLocalPosition &out)
{
	std::vector<std::pair<std::string, std::string>> fields;
	if (!ParseInlineTableFields(value, fields) ||
		!InlineFieldsOnlyContain(fields, { "x", "y" })) {
		return InlineShapeStatus::Unsupported;
	}

	std::string xValue;
	std::string yValue;
	double x = 0.0;
	double y = 0.0;
	if (!FindInlineField(fields, "x", xValue) ||
		!FindInlineField(fields, "y", yValue)) {
		return InlineShapeStatus::Unsupported;
	}
	if (!ParseDouble(xValue, x) || !ParseDouble(yValue, y)) {
		return InlineShapeStatus::WrongType;
	}

	out.present = true;
	out.x = x;
	out.y = y;
	return InlineShapeStatus::Ok;
}

InlineShapeStatus ParseCellBounds(
	const std::string &value,
	RuntimeGameplayAsciiSourcePlanCellBox &out)
{
	std::vector<std::pair<std::string, std::string>> fields;
	if (!ParseInlineTableFields(value, fields) ||
		!InlineFieldsOnlyContain(fields, { "min_x", "min_y", "max_x", "max_y" })) {
		return InlineShapeStatus::Unsupported;
	}

	std::string minXValue;
	std::string minYValue;
	std::string maxXValue;
	std::string maxYValue;
	double minX = 0.0;
	double minY = 0.0;
	double maxX = 0.0;
	double maxY = 0.0;
	if (!FindInlineField(fields, "min_x", minXValue) ||
		!FindInlineField(fields, "min_y", minYValue) ||
		!FindInlineField(fields, "max_x", maxXValue) ||
		!FindInlineField(fields, "max_y", maxYValue)) {
		return InlineShapeStatus::Unsupported;
	}
	if (!ParseDouble(minXValue, minX) || !ParseDouble(minYValue, minY) ||
		!ParseDouble(maxXValue, maxX) || !ParseDouble(maxYValue, maxY)) {
		return InlineShapeStatus::WrongType;
	}

	out.present = true;
	out.minX = minX;
	out.minY = minY;
	out.maxX = maxX;
	out.maxY = maxY;
	return InlineShapeStatus::Ok;
}

void AddWrongType(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &key,
	const IssueContext &context)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType;
	issue.line = line;
	issue.key = key;
	ApplyContext(issue, context);
	AddIssue(result, issue);
}

void AddInvalidGlyph(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &key,
	const IssueContext &context)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString;
	issue.line = line;
	issue.key = key;
	ApplyContext(issue, context);
	AddIssue(result, issue);
}

void AddUnknownEnum(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &key,
	const std::string &value,
	const IssueContext &context)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnknownEnumValue;
	issue.line = line;
	issue.key = key;
	issue.detail = value;
	ApplyContext(issue, context);
	AddIssue(result, issue);
}

void AddSyntax(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &detail,
	const IssueContext &context = {})
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError;
	issue.line = line;
	issue.detail = detail;
	ApplyContext(issue, context);
	AddIssue(result, issue);
}

void AddUnsupported(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &detail,
	const IssueContext &context,
	const std::string &key = {})
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape;
	issue.line = line;
	issue.key = key;
	issue.detail = detail;
	ApplyContext(issue, context);
	AddIssue(result, issue);
}

void AddInlineShapeIssue(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	std::size_t line,
	const std::string &key,
	InlineShapeStatus status,
	const IssueContext &context)
{
	if (status == InlineShapeStatus::WrongType) {
		AddWrongType(result, line, key, context);
	} else if (status == InlineShapeStatus::Unsupported) {
		AddUnsupported(
			result,
			line,
			"unsupported inline table shape for key: " + key,
			context,
			key);
	}
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

RuntimeGameplayAsciiSourcePlanTomlReadIssue MirroredSourcePlanIssue(
	const RuntimeGameplayAsciiSourcePlanIssue &sourceIssue,
	const RuntimeGameplayAsciiSourcePlanTomlSourceLocations &locations)
{
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid;
	issue.detail = "parsed TOML source plan failed source-plan validation";
	issue.sourceIssue = sourceIssue;

	switch (sourceIssue.code) {
	case RuntimeGameplayAsciiSourcePlanIssueCode::EmptyGlyph:
	case RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateGlyph:
		issue.table = "legend";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "glyph";
		if (sourceIssue.index < locations.legendTableLines.size()) {
			issue.line = locations.legendTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellOutOfBounds:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellGlyphMismatch:
	case RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateAnnotatedCellId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::EmptyActorMarkerId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::EmptyProfileMarkerId:
		issue.table = "cells";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		if (sourceIssue.index < locations.annotatedCellTableLines.size()) {
			issue.line = locations.annotatedCellTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::InvalidRegionBounds:
	case RuntimeGameplayAsciiSourcePlanIssueCode::RegionOutOfBounds:
		issue.table = "regions";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		if (sourceIssue.index < locations.regionTableLines.size()) {
			issue.line = locations.regionTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingNpcId:
		issue.table = "frame_controls";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "npc";
		if (sourceIssue.index < locations.frameControlTableLines.size()) {
			issue.line = locations.frameControlTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlUnsupportedBehavior:
		issue.table = "frame_controls";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "behavior";
		if (sourceIssue.index < locations.frameControlTableLines.size()) {
			issue.line = locations.frameControlTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlUnsupportedMoveMode:
		issue.table = "frame_controls";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "move_mode";
		if (sourceIssue.index < locations.frameControlTableLines.size()) {
			issue.line = locations.frameControlTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingTarget:
		issue.table = "frame_controls";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "target";
		if (sourceIssue.index < locations.frameControlTableLines.size()) {
			issue.line = locations.frameControlTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileMissingId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileDuplicateId:
		issue.table = "profiles";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "id";
		if (sourceIssue.index < locations.profileTableLines.size()) {
			issue.line = locations.profileTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileInvalidTraits:
		issue.table = "profiles";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "traits";
		if (sourceIssue.index < locations.profileTableLines.size()) {
			issue.line = locations.profileTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetMissingId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetDuplicateId:
		issue.table = "interaction_targets";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "target_id";
		if (sourceIssue.index < locations.interactionTargetTableLines.size()) {
			issue.line = locations.interactionTargetTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetUnsupportedKind:
		issue.table = "interaction_targets";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "kind";
		if (sourceIssue.index < locations.interactionTargetTableLines.size()) {
			issue.line = locations.interactionTargetTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetMissingPosition:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetPositionOutOfBounds:
		issue.table = "interaction_targets";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "position";
		if (sourceIssue.index < locations.interactionTargetTableLines.size()) {
			issue.line = locations.interactionTargetTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetInvalidRadius:
		issue.table = "interaction_targets";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "radius";
		if (sourceIssue.index < locations.interactionTargetTableLines.size()) {
			issue.line = locations.interactionTargetTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetUnsupportedEffect:
		issue.table = "interaction_targets";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "effect";
		if (sourceIssue.index < locations.interactionTargetTableLines.size()) {
			issue.line = locations.interactionTargetTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingDropId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropDuplicateDropId:
		issue.table = "item_drops";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "drop_id";
		if (sourceIssue.index < locations.itemDropTableLines.size()) {
			issue.line = locations.itemDropTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingItemId:
		issue.table = "item_drops";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "item_id";
		if (sourceIssue.index < locations.itemDropTableLines.size()) {
			issue.line = locations.itemDropTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropInvalidCount:
		issue.table = "item_drops";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "count";
		if (sourceIssue.index < locations.itemDropTableLines.size()) {
			issue.line = locations.itemDropTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingPosition:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropPositionOutOfBounds:
		issue.table = "item_drops";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "position";
		if (sourceIssue.index < locations.itemDropTableLines.size()) {
			issue.line = locations.itemDropTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropInvalidPickupRadius:
		issue.table = "item_drops";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "pickup_radius";
		if (sourceIssue.index < locations.itemDropTableLines.size()) {
			issue.line = locations.itemDropTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		AuthoredPlayerCommandUnsupportedCommand:
		issue.table = "frame_player_commands";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "command";
		if (sourceIssue.index < locations.framePlayerCommandTableLines.size()) {
			issue.line = locations.framePlayerCommandTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		AuthoredPlayerCommandMissingTarget:
		issue.table = "frame_player_commands";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "target";
		if (sourceIssue.index < locations.framePlayerCommandTableLines.size()) {
			issue.line = locations.framePlayerCommandTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		AuthoredPlayerCommandUnknownInteractionTarget:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		AuthoredPlayerCommandInvalidPickupTarget:
		issue.table = "frame_player_commands";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "target_id";
		if (sourceIssue.index < locations.framePlayerCommandTableLines.size()) {
			issue.line = locations.framePlayerCommandTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedFinalRowsEmpty:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedFinalRowsDimensionMismatch:
		issue.table = "expect";
		issue.key = "final_rows";
		issue.line = locations.expectFinalRowsLine != 0
			? locations.expectFinalRowsLine
			: locations.expectTableLine;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedTraceFrameRowsEmpty:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedTraceFrameRowsDimensionMismatch:
		issue.table = "expect_trace_frames";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "rows";
		if (sourceIssue.index <
			locations.expectTraceFrameRowsLines.size()) {
			issue.line = locations.expectTraceFrameRowsLines[sourceIssue.index] != 0
				? locations.expectTraceFrameRowsLines[sourceIssue.index]
				: locations.expectTraceFrameTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInventoryStackMissingItemId:
		issue.table = "expect_inventory_stacks";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "item_id";
		if (sourceIssue.index <
			locations.expectInventoryStackTableLines.size()) {
			issue.line = locations.expectInventoryStackTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInventoryStackInvalidCount:
		issue.table = "expect_inventory_stacks";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "count";
		if (sourceIssue.index <
			locations.expectInventoryStackTableLines.size()) {
			issue.line = locations.expectInventoryStackTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInventoryStackDuplicateItemId:
		issue.table = "expect_inventory_stacks";
		issue.hasTableIndex = true;
		issue.tableIndex = sourceIssue.index;
		issue.key = "item_id";
		if (sourceIssue.index <
			locations.expectInventoryStackTableLines.size()) {
			issue.line = locations.expectInventoryStackTableLines[sourceIssue.index];
		}
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::EmptyRows:
	case RuntimeGameplayAsciiSourcePlanIssueCode::GridDimensionMismatch:
	case RuntimeGameplayAsciiSourcePlanIssueCode::RaggedRow:
	case RuntimeGameplayAsciiSourcePlanIssueCode::UnknownGridGlyph:
		issue.table = "grid";
		issue.line = locations.gridRowsLine != 0
			? locations.gridRowsLine
			: locations.gridTableLine;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::UnsafeNoClaims:
		issue.table = "no_claims";
		issue.line = locations.noClaimsTableLine;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::UnsafePromotionPolicy:
		issue.table = "promotion";
		issue.line = locations.promotionTableLine;
		break;
	}

	return issue;
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
	IssueContext context;
	bool sawGrid = false;
	MultilineStringArrayTarget multilineTarget =
		MultilineStringArrayTarget::None;
	std::size_t authoredDeclarationIndex = 0;
	std::vector<std::string> parsedRows;

	const std::vector<std::string> lines = Lines(text);
	for (std::size_t index = 0; index < lines.size(); ++index) {
		const std::size_t lineNumber = index + 1;
		const std::string line = Trim(StripComment(lines[index]));
		if (line.empty()) {
			continue;
		}

		if (multilineTarget != MultilineStringArrayTarget::None) {
			std::string row;
			bool closed = false;
			if (!ParseMultilineStringArrayItem(line, row, closed)) {
				AddWrongType(
					result,
					lineNumber,
					multilineTarget == MultilineStringArrayTarget::GridRows
						? "rows"
						: multilineTarget ==
								MultilineStringArrayTarget::ExpectFinalRows
							? "final_rows"
							: "rows",
					context);
				continue;
			}
			if (closed) {
				if (multilineTarget == MultilineStringArrayTarget::GridRows) {
					result.plan.grid.rows = parsedRows;
				} else if (
					multilineTarget ==
					MultilineStringArrayTarget::ExpectFinalRows) {
					result.plan.expectations.hasFinalRows = true;
					result.plan.expectations.finalRows = parsedRows;
				} else if (!result.plan.expectations.traceFrames.empty()) {
					RuntimeGameplayAsciiSourcePlanExpectedTraceFrame &frame =
						result.plan.expectations.traceFrames.back();
					frame.hasRows = true;
					frame.rows = parsedRows;
				}
				multilineTarget = MultilineStringArrayTarget::None;
			} else {
				parsedRows.push_back(row);
			}
			continue;
		}

		if (line == "[grid]") {
			table = Table::Grid;
			context = { table, false, 0 };
			sawGrid = true;
			result.sourceLocations.gridTableLine = lineNumber;
			continue;
		}
		if (line == "[no_claims]") {
			table = Table::NoClaims;
			context = { table, false, 0 };
			result.sourceLocations.noClaimsTableLine = lineNumber;
			continue;
		}
		if (line == "[promotion]") {
			table = Table::Promotion;
			context = { table, false, 0 };
			result.sourceLocations.promotionTableLine = lineNumber;
			continue;
		}
		if (line == "[expect]") {
			table = Table::Expect;
			context = { table, false, 0 };
			result.sourceLocations.expectTableLine = lineNumber;
			continue;
		}
		if (line == "[[expect_trace_frames]]") {
			result.plan.expectations.traceFrames.push_back({});
			result.sourceLocations.expectTraceFrameTableLines.push_back(lineNumber);
			result.sourceLocations.expectTraceFrameRowsLines.push_back(0);
			table = Table::ExpectTraceFrames;
			context = {
				table,
				true,
				result.plan.expectations.traceFrames.size() - 1,
			};
			continue;
		}
		if (line == "[[expect_inventory_stacks]]") {
			result.plan.expectations.inventoryStacks.push_back({});
			result.sourceLocations.expectInventoryStackTableLines.push_back(
				lineNumber);
			table = Table::ExpectInventoryStacks;
			context = {
				table,
				true,
				result.plan.expectations.inventoryStacks.size() - 1,
			};
			continue;
		}
		if (line == "[[legend]]") {
			result.plan.legend.push_back({});
			result.sourceLocations.legendTableLines.push_back(lineNumber);
			table = Table::Legend;
			context = { table, true, result.plan.legend.size() - 1 };
			continue;
		}
		if (line == "[[cells]]") {
			result.plan.annotatedCells.push_back({});
			result.sourceLocations.annotatedCellTableLines.push_back(lineNumber);
			table = Table::Cells;
			context = { table, true, result.plan.annotatedCells.size() - 1 };
			continue;
		}
		if (line == "[[regions]]") {
			result.plan.regions.push_back({});
			result.sourceLocations.regionTableLines.push_back(lineNumber);
			table = Table::Regions;
			context = { table, true, result.plan.regions.size() - 1 };
			continue;
		}
		if (line == "[[profiles]]") {
			result.plan.authoredProfiles.push_back({});
			result.sourceLocations.profileTableLines.push_back(lineNumber);
			table = Table::Profiles;
			context = { table, true, result.plan.authoredProfiles.size() - 1 };
			continue;
		}
		if (line == "[[interaction_targets]]") {
			result.plan.authoredInteractionTargets.push_back({});
			result.sourceLocations.interactionTargetTableLines.push_back(
				lineNumber);
			table = Table::InteractionTargets;
			context = {
				table,
				true,
				result.plan.authoredInteractionTargets.size() - 1,
			};
			continue;
		}
		if (line == "[[item_drops]]") {
			result.plan.authoredItemDrops.push_back({});
			result.sourceLocations.itemDropTableLines.push_back(lineNumber);
			table = Table::ItemDrops;
			context = {
				table,
				true,
				result.plan.authoredItemDrops.size() - 1,
			};
			continue;
		}
		if (line == "[[frame_controls]]") {
			result.plan.authoredControls.push_back({});
			result.plan.authoredControls.back().hasDeclarationIndex = true;
			result.plan.authoredControls.back().declarationIndex =
				authoredDeclarationIndex++;
			result.sourceLocations.frameControlTableLines.push_back(lineNumber);
			table = Table::FrameControls;
			context = { table, true, result.plan.authoredControls.size() - 1 };
			continue;
		}
		if (line == "[[frame_player_commands]]") {
			result.plan.authoredPlayerCommands.push_back({});
			result.plan.authoredPlayerCommands.back().hasDeclarationIndex = true;
			result.plan.authoredPlayerCommands.back().declarationIndex =
				authoredDeclarationIndex++;
			result.sourceLocations.framePlayerCommandTableLines.push_back(
				lineNumber);
			table = Table::FramePlayerCommands;
			context = {
				table,
				true,
				result.plan.authoredPlayerCommands.size() - 1,
			};
			continue;
		}
		if (!line.empty() && line.front() == '[') {
			AddUnsupported(
				result,
				lineNumber,
				"unsupported TOML table in source-plan reader slice",
				context);
			continue;
		}

		const std::size_t equals = line.find('=');
		if (equals == std::string::npos) {
			AddSyntax(result, lineNumber, "expected key = value", context);
			continue;
		}
		const std::string key = Trim(std::string_view(line).substr(0, equals));
		const std::string value = Trim(std::string_view(line).substr(equals + 1));
		if (key.empty()) {
			AddSyntax(result, lineNumber, "empty key", context);
			continue;
		}

		if (table == Table::Root) {
			if (key == "format_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					result.plan.formatId = ResourceId(parsed);
				}
			} else if (key == "version") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					result.plan.version = parsed;
				}
			} else if (key == "source_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					result.plan.hasSourceId = true;
					result.plan.sourceId = ResourceId(parsed);
				}
			} else if (key == "source_ref") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					result.plan.hasSourceRef = true;
					result.plan.sourceRef = ResourceId(parsed);
				}
			} else {
				AddUnsupported(result, lineNumber, "unsupported root key: " + key, context, key);
			}
			continue;
		}

		if (table == Table::NoClaims) {
			bool parsed = false;
			if (!ParseBool(value, parsed)) {
				AddWrongType(result, lineNumber, key, context);
			} else if (key == "runtime_truth") {
				result.plan.noClaims.claimsRuntimeTruth = parsed;
			} else if (key == "gameplay_execution") {
				result.plan.noClaims.claimsGameplayExecution = parsed;
			} else if (key == "file_parsing") {
				result.plan.noClaims.claimsFileParsing = parsed;
			} else if (key == "profile_scenario_conversion") {
				result.plan.noClaims.claimsProfileScenarioConversion = parsed;
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported no_claims key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::Promotion) {
			bool parsed = false;
			if (!ParseBool(value, parsed)) {
				AddWrongType(result, lineNumber, key, context);
			} else if (key == "ready") {
				result.plan.promotionPolicy.promotionReady = parsed;
			} else if (key == "runtime_execution") {
				result.plan.promotionPolicy.allowsRuntimeExecution = parsed;
			} else if (key == "file_parsing") {
				result.plan.promotionPolicy.allowsFileParsing = parsed;
			} else if (key == "profile_scenario_conversion") {
				result.plan.promotionPolicy.allowsProfileScenarioConversion = parsed;
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported promotion key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::Expect) {
			RuntimeGameplayAsciiSourcePlanExpectations &expectations =
				result.plan.expectations;
			if (key == "final_rows") {
				result.sourceLocations.expectFinalRowsLine = lineNumber;
				if (value == "[") {
					multilineTarget =
						MultilineStringArrayTarget::ExpectFinalRows;
					parsedRows.clear();
					continue;
				}
				std::vector<std::string> rows;
				if (!ParseStringArrayInline(value, rows)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					expectations.hasFinalRows = true;
					expectations.finalRows = rows;
				}
			} else if (key == "frame_count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					expectations.hasFrameCount = true;
					expectations.frameCount = parsed;
				}
			} else if (key == "accepted_command_count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					expectations.hasAcceptedCommandCount = true;
					expectations.acceptedCommandCount = parsed;
				}
			} else if (key == "picked_up_count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					expectations.hasPickedUpCount = true;
					expectations.pickedUpCount = parsed;
				}
			} else if (key == "interaction_changed") {
				bool parsed = false;
				if (!ParseBool(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					expectations.hasInteractionChanged = true;
					expectations.interactionChanged = parsed;
				}
			} else if (key == "npc_moved_count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					expectations.hasNpcMovedCount = true;
					expectations.npcMovedCount = parsed;
				}
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported expect key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::ExpectTraceFrames) {
			RuntimeGameplayAsciiSourcePlanExpectedTraceFrame &frame =
				result.plan.expectations.traceFrames.back();
			if (key == "frame_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					frame.hasFrameId = true;
					frame.frameId = ResourceId(parsed);
				}
			} else if (key == "rows") {
				if (context.tableIndex <
					result.sourceLocations.expectTraceFrameRowsLines.size()) {
					result.sourceLocations
						.expectTraceFrameRowsLines[context.tableIndex] =
						lineNumber;
				}
				if (value == "[") {
					multilineTarget =
						MultilineStringArrayTarget::ExpectTraceFrameRows;
					parsedRows.clear();
					continue;
				}
				std::vector<std::string> rows;
				if (!ParseStringArrayInline(value, rows)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					frame.hasRows = true;
					frame.rows = rows;
				}
			} else if (key == "accepted_command_count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					frame.hasAcceptedCommandCount = true;
					frame.acceptedCommandCount = parsed;
				}
			} else if (key == "picked_up_count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					frame.hasPickedUpCount = true;
					frame.pickedUpCount = parsed;
				}
			} else if (key == "interaction_changed") {
				bool parsed = false;
				if (!ParseBool(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					frame.hasInteractionChanged = true;
					frame.interactionChanged = parsed;
				}
			} else if (key == "npc_moved_count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					frame.hasNpcMovedCount = true;
					frame.npcMovedCount = parsed;
				}
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported expect_trace_frames key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::ExpectInventoryStacks) {
			RuntimeGameplayAsciiSourcePlanExpectedInventoryStack &stack =
				result.plan.expectations.inventoryStacks.back();
			if (key == "item_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					stack.itemId = ResourceId(parsed);
				}
			} else if (key == "count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed) ||
					parsed > std::numeric_limits<std::uint32_t>::max()) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					stack.count = static_cast<std::uint32_t>(parsed);
				}
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported expect_inventory_stacks key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::Legend) {
			RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry =
				result.plan.legend.back();
			if (key == "glyph") {
				char glyph = '\0';
				if (!ParseGlyph(value, glyph)) {
					AddInvalidGlyph(result, lineNumber, key, context);
				} else {
					entry.glyph = glyph;
				}
			} else if (key == "kind") {
				std::string parsed;
				RuntimeGameplayAsciiSourcePlanGlyphKind kind =
					RuntimeGameplayAsciiSourcePlanGlyphKind::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else if (!ParseGlyphKind(parsed, kind)) {
					AddUnknownEnum(result, lineNumber, key, parsed, context);
				} else {
					entry.kind = kind;
				}
			} else if (key == "role_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					entry.roleId = ResourceId(parsed);
				}
			} else if (key == "role_tags") {
				std::vector<std::string> parsed;
				if (!ParseStringArrayInline(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					entry.roleTags = ToResourceIds(parsed);
				}
			} else if (key == "maps_to_scenario_marker") {
				bool parsed = false;
				if (!ParseBool(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					entry.mapsToScenarioMarker = parsed;
				}
			} else if (key == "scenario_marker_kind") {
				std::string parsed;
				RuntimeGameplayAsciiScenarioMarkerKind kind =
					RuntimeGameplayAsciiScenarioMarkerKind::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else if (!ParseMarkerKind(parsed, kind)) {
					AddUnknownEnum(result, lineNumber, key, parsed, context);
				} else {
					entry.scenarioMarkerKind = kind;
				}
			} else if (key == "target_marker_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					entry.targetMarkerId = ResourceId(parsed);
				}
			} else if (key == "target_profile_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					entry.targetProfileId = ResourceId(parsed);
				}
			} else {
				AddUnsupported(result, lineNumber, "unsupported legend key: " + key, context, key);
			}
			continue;
		}

		if (table == Table::Cells) {
			RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell =
				result.plan.annotatedCells.back();
			if (key == "id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					cell.hasCellId = true;
					cell.cellId = ResourceId(parsed);
				}
			} else if (key == "row") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					cell.row = parsed;
				}
			} else if (key == "column") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					cell.column = parsed;
				}
			} else if (key == "glyph") {
				char glyph = '\0';
				if (!ParseGlyph(value, glyph)) {
					AddInvalidGlyph(result, lineNumber, key, context);
				} else {
					cell.glyph = glyph;
				}
			} else if (key == "local_tile") {
				const InlineShapeStatus status = ParseLocalTile(value, cell.localTile);
				AddInlineShapeIssue(result, lineNumber, key, status, context);
			} else if (key == "local_position") {
				const InlineShapeStatus status =
					ParseLocalPosition(value, cell.localPosition);
				AddInlineShapeIssue(result, lineNumber, key, status, context);
			} else if (key == "cell_bounds") {
				const InlineShapeStatus status = ParseCellBounds(value, cell.cellBounds);
				AddInlineShapeIssue(result, lineNumber, key, status, context);
			} else if (key == "role_tags") {
				std::vector<std::string> parsed;
				if (!ParseStringArrayInline(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					cell.roleTags = ToResourceIds(parsed);
				}
			} else if (key == "marker_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					cell.markerId = ResourceId(parsed);
				}
			} else if (key == "profile_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					cell.profileId = ResourceId(parsed);
				}
			} else {
				AddUnsupported(result, lineNumber, "unsupported cells key: " + key, context, key);
			}
			continue;
		}

		if (table == Table::Regions) {
			RuntimeGameplayAsciiSourcePlanRegion &region = result.plan.regions.back();
			if (key == "id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					region.hasRegionId = true;
					region.regionId = ResourceId(parsed);
				}
			} else if (key == "min_row") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					region.minRow = parsed;
				}
			} else if (key == "min_column") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					region.minColumn = parsed;
				}
			} else if (key == "max_row") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					region.maxRow = parsed;
				}
			} else if (key == "max_column") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					region.maxColumn = parsed;
				}
			} else if (key == "role_tags") {
				std::vector<std::string> parsed;
				if (!ParseStringArrayInline(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					region.roleTags = ToResourceIds(parsed);
				}
			} else {
				AddUnsupported(result, lineNumber, "unsupported regions key: " + key, context, key);
			}
			continue;
		}

		if (table == Table::Profiles) {
			RuntimeGameplayAsciiSourcePlanAuthoredProfile &profile =
				result.plan.authoredProfiles.back();
			if (key == "id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					profile.profileId = ResourceId(parsed);
				}
			} else if (key == "strength") {
				int parsed = 0;
				if (!ParseSigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					profile.traits.strength = parsed;
				}
			} else if (key == "dexterity") {
				int parsed = 0;
				if (!ParseSigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					profile.traits.dexterity = parsed;
				}
			} else if (key == "constitution") {
				int parsed = 0;
				if (!ParseSigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					profile.traits.constitution = parsed;
				}
			} else if (key == "intelligence") {
				int parsed = 0;
				if (!ParseSigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					profile.traits.intelligence = parsed;
				}
			} else if (key == "wisdom") {
				int parsed = 0;
				if (!ParseSigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					profile.traits.wisdom = parsed;
				}
			} else if (key == "charisma") {
				int parsed = 0;
				if (!ParseSigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					profile.traits.charisma = parsed;
				}
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported profiles key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::InteractionTargets) {
			RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &target =
				result.plan.authoredInteractionTargets.back();
			if (key == "target_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.targetId = ResourceId(parsed);
				}
			} else if (key == "kind") {
				std::string parsed;
				RuntimeGameplayAsciiSourcePlanInteractionTargetKind kind =
					RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else if (!ParseInteractionTargetKind(parsed, kind)) {
					AddUnknownEnum(result, lineNumber, key, parsed, context);
				} else {
					target.kind = kind;
				}
			} else if (key == "tile") {
				const InlineShapeStatus status =
					ParseLocalTile(value, target.localTile);
				AddInlineShapeIssue(result, lineNumber, key, status, context);
			} else if (key == "position") {
				const InlineShapeStatus status =
					ParseLocalPosition(value, target.localPosition);
				AddInlineShapeIssue(result, lineNumber, key, status, context);
			} else if (key == "radius") {
				double parsed = 0.0;
				if (!ParseDouble(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.radius = parsed;
				}
			} else if (key == "enabled") {
				bool parsed = false;
				if (!ParseBool(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.enabled = parsed;
				}
			} else if (key == "effect") {
				std::string parsed;
				RuntimeGameplayAsciiSourcePlanInteractionEffectKind effect =
					RuntimeGameplayAsciiSourcePlanInteractionEffectKind::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else if (!ParseInteractionEffectKind(parsed, effect)) {
					AddUnknownEnum(result, lineNumber, key, parsed, context);
				} else {
					target.effect = effect;
				}
			} else if (key == "effect_target_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.effectTargetId = ResourceId(parsed);
				}
			} else if (key == "event_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.eventId = ResourceId(parsed);
				}
			} else if (key == "drop_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.dropId = ResourceId(parsed);
				}
			} else if (key == "required_item_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.requiredItemId = ResourceId(parsed);
				}
			} else if (key == "text") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.text = parsed;
				}
			} else if (key == "enabled_value") {
				bool parsed = false;
				if (!ParseBool(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					target.enabledValue = parsed;
				}
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported interaction_targets key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::ItemDrops) {
			RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &drop =
				result.plan.authoredItemDrops.back();
			if (key == "drop_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					drop.dropId = ResourceId(parsed);
				}
			} else if (key == "item_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					drop.itemId = ResourceId(parsed);
				}
			} else if (key == "count") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed) ||
					parsed > std::numeric_limits<std::uint32_t>::max()) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					drop.count = static_cast<std::uint32_t>(parsed);
				}
			} else if (key == "tile") {
				const InlineShapeStatus status =
					ParseLocalTile(value, drop.localTile);
				AddInlineShapeIssue(result, lineNumber, key, status, context);
			} else if (key == "position") {
				const InlineShapeStatus status =
					ParseLocalPosition(value, drop.localPosition);
				AddInlineShapeIssue(result, lineNumber, key, status, context);
			} else if (key == "pickup_radius") {
				double parsed = 0.0;
				if (!ParseDouble(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					drop.pickupRadius = parsed;
				}
			} else if (key == "enabled") {
				bool parsed = false;
				if (!ParseBool(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					drop.enabled = parsed;
				}
			} else if (key == "glyph") {
				char glyph = '\0';
				if (!ParseGlyph(value, glyph)) {
					AddInvalidGlyph(result, lineNumber, key, context);
				} else {
					drop.glyph = glyph;
				}
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported item_drops key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::FrameControls) {
			RuntimeGameplayAsciiSourcePlanAuthoredControl &control =
				result.plan.authoredControls.back();
			if (key == "frame_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					control.hasFrameId = true;
					control.frameId = ResourceId(parsed);
				}
			} else if (key == "npc") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					control.npcId = ResourceId(parsed);
				}
			} else if (key == "behavior") {
				std::string parsed;
				RuntimeGameplayAsciiSourcePlanControlBehavior behavior =
					RuntimeGameplayAsciiSourcePlanControlBehavior::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else if (!ParseControlBehavior(parsed, behavior)) {
					AddUnknownEnum(result, lineNumber, key, parsed, context);
				} else {
					control.behavior = behavior;
				}
			} else if (key == "move_mode") {
				std::string parsed;
				RuntimeGameplayAsciiSourcePlanControlMoveMode moveMode =
					RuntimeGameplayAsciiSourcePlanControlMoveMode::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else if (!ParseControlMoveMode(parsed, moveMode)) {
					AddUnknownEnum(result, lineNumber, key, parsed, context);
				} else {
					control.moveMode = moveMode;
				}
			} else if (key == "target") {
				const InlineShapeStatus status =
					ParseLocalPosition(value, control.targetPosition);
				AddInlineShapeIssue(result, lineNumber, key, status, context);
			} else {
				AddUnsupported(result, lineNumber, "unsupported frame_controls key: " + key, context, key);
			}
			continue;
		}

		if (table == Table::FramePlayerCommands) {
			RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand &command =
				result.plan.authoredPlayerCommands.back();
			if (key == "frame_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					command.hasFrameId = true;
					command.frameId = ResourceId(parsed);
				}
			} else if (key == "command") {
				std::string parsed;
				RuntimeGameplayAsciiSourcePlanPlayerCommandKind kind =
					RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Unknown;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else if (!ParsePlayerCommandKind(parsed, kind)) {
					AddUnknownEnum(result, lineNumber, key, parsed, context);
				} else {
					command.command = kind;
				}
			} else if (key == "x") {
				int parsed = 0;
				if (!ParseSigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					command.targetTile.x = parsed;
					command.hasTargetTileX = true;
					command.hasTargetTile =
						command.hasTargetTileX && command.hasTargetTileY;
				}
			} else if (key == "y") {
				int parsed = 0;
				if (!ParseSigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					command.targetTile.y = parsed;
					command.hasTargetTileY = true;
					command.hasTargetTile =
						command.hasTargetTileX && command.hasTargetTileY;
				}
			} else if (key == "target_id") {
				std::string parsed;
				if (!ParseQuotedString(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					command.targetId = ResourceId(parsed);
				}
			} else {
				AddUnsupported(
					result,
					lineNumber,
					"unsupported frame_player_commands key: " + key,
					context,
					key);
			}
			continue;
		}

		if (table == Table::Grid) {
			if (key == "width") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					result.plan.grid.width = parsed;
				}
			} else if (key == "height") {
				std::size_t parsed = 0;
				if (!ParseUnsigned(value, parsed)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					result.plan.grid.height = parsed;
				}
			} else if (key == "background") {
				char glyph = '\0';
				if (!ParseGlyph(value, glyph)) {
					AddInvalidGlyph(result, lineNumber, key, context);
				} else {
					result.plan.grid.backgroundGlyph = glyph;
				}
			} else if (key == "rows") {
				result.sourceLocations.gridRowsLine = lineNumber;
				if (value == "[") {
					multilineTarget = MultilineStringArrayTarget::GridRows;
					parsedRows.clear();
					continue;
				}
				std::vector<std::string> rows;
				if (!ParseStringArrayInline(value, rows)) {
					AddWrongType(result, lineNumber, key, context);
				} else {
					result.plan.grid.rows = rows;
				}
			} else {
				AddUnsupported(result, lineNumber, "unsupported grid key: " + key, context, key);
			}
		}
	}

	if (multilineTarget != MultilineStringArrayTarget::None) {
		AddSyntax(result, lines.size(), "unterminated string array", context);
	}

	if (!sawGrid) {
		AddMissingTable(result, "grid");
	}

	if (!result.issues.empty()) {
		result.status = StatusForParserIssues(result);
		return result;
	}

	result.sourceValidation = RuntimeGameplayAsciiSourcePlanValidator {}.validate(result.plan);
	if (!result.sourceValidation.ok()) {
		for (const RuntimeGameplayAsciiSourcePlanIssue &sourceIssue :
			result.sourceValidation.issues) {
			AddIssue(result, MirroredSourcePlanIssue(sourceIssue, result.sourceLocations));
		}
		result.status = RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid;
		return result;
	}

	result.status = RuntimeGameplayAsciiSourcePlanTomlReadStatus::Parsed;
	return result;
}

} // namespace iggy::runtime
