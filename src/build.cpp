#include <cctype>
#include <filesystem>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <toml++/toml.hpp>

#include "compiler.h"
#include "io.h"
#include "manifest.h"

// std::filesystem::path outfile = std::filesystem::absolute("main");
// std::vector<std::filesystem::path> infiles;

namespace freight { // namespace
	err::Failure fail_parsing_manifest(const std::filesystem::path& file) {
		namespace fs = std::filesystem;

		return err::fail(std::format(
		  "failed to parse manifest at `{}`", fs::absolute(file).string()));
	}

	namespace parse {
		namespace {
			bool _is_digits(std::string_view str) {
				for (char ch : str)
					if (!std::isdigit(ch))
						return false;

				return true;
			}
		} // namespace

		bool validate_standard(std::string_view str) {
			if (str.size() != 5 || str.substr(0, 3) != "c++")
				return false;

			std::string num_str {str.substr(3, 5)};
			if (!_is_digits(num_str))
				return false;

			int num = std::stoi(num_str);
			switch (num) {
			case 03:
			case 11:
			case 14:
			case 17:
			case 20:
			case 23:
				return true;
			default:
				return false;
			}
		}
	} // namespace parse

	Manifest read_manifest(const std::filesystem::path& path) {
		toml::table tbl;
		try {
			tbl = toml::parse_file(path.native());
		} catch (const toml::parse_error& err) {
			fail_parsing_manifest(path).cause(err.what());
		}

		auto proj = tbl["project"];
		if (!proj.is_value()) {
			fail_parsing_manifest(path).cause("manifest is missing [project]");
		}

		Manifest manifest;

		// error checking
		auto name_node = proj["name"];
		if (!name_node.is_string()) {
			err::fail("`project.name`: invalid type: expected a string").exit();
		}

		auto standard_node = proj["standard"];
		if (!standard_node.is_integer()) {
			err::fail("`project.standard`: invalid type: expected an integer")
			  .exit();
		}

		auto version_node = proj["version"];
		if (!version_node.is_string()) {
			err::fail("`project.version`: invalid type: expected a string")
			  .exit();
		}

		// name ------------------------------------------------------------
		auto name_opt = name_node.value<std::string>();
		if (!name_opt.has_value()) {
			fail_parsing_manifest(path).cause("missing field `project.name`");
		}

		manifest.name = name_opt.value();

		// standard ------------------------------------------------------------
		auto std_opt = standard_node.value<std::string>();
		if (std_opt.has_value()) {
			auto std = std_opt.value();
			if (!parse::validate_standard(std)) {
				fail_parsing_manifest(path)
				  .cause("failed to parse the `standard` key")
				  .cause(
					std::format("reccommended standard values are `c++11`, "
								"`c++17`, or `c++20`, but `{}` is unknown",
					  std));
			}

			manifest.standard = std_opt.value();
		} else {
			io::warning("no standard set: defaulting to C++20 while the latest "
						"is C++2023");

			manifest.standard = "c++20";
		}

		// version ------------------------------------------------------------
		auto static const DEFAULT_VERSION = "0.0.0";
		auto version_opt = proj["version"].value<std::string>();
		manifest.version = version_opt.value_or(DEFAULT_VERSION);

		return manifest;
	}

	Manifest load_manifest(const std::filesystem::path& path) {
		namespace fs = std::filesystem;

		auto manifest_path = path / Manifest::FILENAME;
		if (!fs::exists(manifest_path)) {
			throw std::invalid_argument {
			  std::format("could not find manifest: file \"{}\" does not exist",
				fs::absolute(manifest_path).string())};
		}

		return read_manifest(manifest_path);
	}

	struct BuildOptions {
		std::filesystem::path path;
		bool release;
	};

	bool build(const BuildOptions& opts) {
		namespace fs = std::filesystem;

		auto manifest_path = opts.path / Manifest::FILENAME;
		if (!fs::exists(manifest_path)) {
			err::fail(std::format("could not find `{}` in `{}`",
						manifest_path.filename().string(),
						opts.path.string()))
			  .exit();
		}

		Manifest manifest = load_manifest(manifest_path);

		auto compiler = freight::Compiler::get();
		if (!compiler) {
			err::fail("failed to build files")
			  .cause("could not locate a C++ compiler")
			  .exit();
		}

		CompileOpts buildopts {.std = manifest.standard};

		/*
		for file in "src"
		{
			compiler.compile(opts, file)
		}
		*/

		return true;
	}
} // namespace freight