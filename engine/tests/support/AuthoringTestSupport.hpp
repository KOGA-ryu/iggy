#pragma once

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <vector>

namespace iggy::test {

struct AuthoringCliCommandResult {
	int exitCode = -1;
	std::string output;
};

inline bool Contains(std::string_view text, std::string_view needle)
{
	return text.find(needle) != std::string_view::npos;
}

inline std::filesystem::path AuthoringSourceFixturePath(
	const char *fixtureRoot,
	const char *name)
{
	return std::filesystem::path(fixtureRoot) / name;
}

inline std::filesystem::path AuthoringPackageFixturePath(
	const char *fixtureRoot,
	const char *name)
{
	return std::filesystem::path(fixtureRoot) / name;
}

inline std::string AuthoringSourceFixturePathString(
	const char *fixtureRoot,
	const char *name)
{
	return AuthoringSourceFixturePath(fixtureRoot, name).string();
}

inline std::string AuthoringPackageFixturePathString(
	const char *fixtureRoot,
	const char *name)
{
	return AuthoringPackageFixturePath(fixtureRoot, name).string();
}

inline std::string AuthoringSourceFixtureText(
	const char *fixtureRoot,
	const char *name)
{
	std::ifstream stream(AuthoringSourceFixturePath(fixtureRoot, name));
	std::ostringstream text;
	text << stream.rdbuf();
	return text.str();
}

inline std::string FinalRowsBlock(const std::vector<std::string> &rows)
{
	std::ostringstream stream;
	stream << "final_rows:\n";
	for (const std::string &row : rows)
		stream << row << '\n';
	return stream.str();
}

inline std::string ShellQuote(std::string_view value)
{
	std::string quoted = "'";
	for (char ch : value) {
		if (ch == '\'')
			quoted += "'\\''";
		else
			quoted += ch;
	}
	quoted += "'";
	return quoted;
}

inline int DecodeExitCode(int status)
{
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}

inline AuthoringCliCommandResult RunAuthoringCli(
	const char *runnerPath,
	const std::vector<std::string> &args)
{
	std::string command = ShellQuote(runnerPath);
	for (const std::string &arg : args) {
		command += ' ';
		command += ShellQuote(arg);
	}
	command += " 2>&1";

	AuthoringCliCommandResult result;
	std::array<char, 256> buffer {};
	FILE *pipe = popen(command.c_str(), "r");
	if (pipe == nullptr) {
		result.output = "popen failed";
		return result;
	}
	while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
		result.output += buffer.data();
	result.exitCode = DecodeExitCode(pclose(pipe));
	return result;
}

inline void ExpectOutputContains(
	const AuthoringCliCommandResult &result,
	const std::vector<std::string> &needles,
	std::string_view context,
	int &failures)
{
	for (const std::string &needle : needles) {
		if (!Contains(result.output, needle)) {
			std::cerr << "FAIL: " << context << " missing: " << needle
				<< "\noutput:\n" << result.output << '\n';
			++failures;
		}
	}
}

} // namespace iggy::test
