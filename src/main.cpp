#include <string>
#define _POSIX_C_SOURCE 200809L

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <print>
#include <stdexcept>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// class Project
// {
//   private:
//     // Member variables
//     std::filesystem::path _base;
//     Manifest _manifest;
//     bool _debug = true;

//     /**
//      * @brief Creates a file with the name "Proj.toml" at
//      `this->base_path()`.
//      * The file follows the TOML specification.
//      * The contents of this file are as follows:
//      * ~~~{.toml}
//      * [project]
//      * name = "projname"
//      * version = "ver"
//      * standard = "std"
//      * ~~~
//      * Where `projname` is the name of the project, `ver` its version, and
//      `std`
//      * is its C++ standard.
//      * @return the path of the file if it was successfully created
//      * @return an empty path if the file could not be created
//      */

//     /**
//      * @brief Creates a source file with the name "main.cpp" at
//      * `this->src_path()`. The content of this file is the code of a generic
//      C++
//      * "Hello World!" program.
//      * @return the path of the file if it was successfully created
//      * @return an empty path if the file could not be created
//      */
//   public:
//     static std::optional<Project>
//     load_existing(const std::filesystem::path& manifest_dir)
//     {
//         namespace fs = std::filesystem;

//         auto manifest_file = manifest_dir / "Proj.toml";
//         if (!fs::exists(manifest_file))
//         {
//             std::println(stderr, "error: could not find `{}` in `{}`",
//                          manifest_file.filename(), manifest_dir);
//             return {};
//         }

//         Manifest manifest = Manifest::from_file(manifest_file);
//         Project project{manifest_dir, std::move(manifest)};
//         return project;
//     }

//     const std::string& name() { return _manifest.name; }

//     std::string version() { return "0.1.0"; }

//     int standard() { return 20; }

//     bool debug() { return _debug; }

//     bool release() { return !_debug; }

//     const std::filesystem::path& base_path() { return _base; }

//     std::filesystem::path target_path()
//     {
//         return base_path() / "target" / (debug() ? "debug" : "release");
//     }

//     std::filesystem::path src_path() { return base_path() / "src"; }

//     bool build()
//     {
//         // invoke compiler
//         return false;
//     }

//     bool run()
//     {
//         // invoke compiler
//         return false;
//     }
// };

std::string help_message() { return {}; }

void print_help() {}

const auto USAGE_FOR_DOCOPTS = R"(
Usage:
  freight new <project>
  freight build [-r|--release]
  freight run

Options:
  -h --help  
)";

int build_tool_main()
{
    // if (args.size() == 0)
    // {
    //     std::cout << freight::help_message();
    //     return EXIT_SUCCESS;
    // }

    // if (args.size() > 0)
    // {
    //     if (index_of(args, "--help") != -1)
    //     {
    //         std::cout << freight::help_message();
    //         return EXIT_SUCCESS;
    //     }
    //     else if (args[1] == "new")
    //     {
    //         if (args.size() < 2 || args.size() > 2)
    //         {
    //             if (args.size() < 2)
    //             {
    //                 std::cerr << "Not enough arguments";
    //             }
    //             else if (args.size() > 2)
    //             {
    //                 std::cerr << "Too many arguments";
    //             }

    //             std::cerr << "Command syntax: cppbt new <projectname>";
    //             return EXIT_FAILURE;
    //         }

    //         auto project = freight::Project::new_project(args[2]);
    //         if (!project.has_value())
    //         {
    //             return EXIT_FAILURE;
    //         }
    //     }
    //     else if (args[1] == "build")
    //     {
    //         if (args.size() > 2)
    //         {
    //             std::cerr << "Too many arguments";
    //             return EXIT_FAILURE;
    //         }

    //         auto project =
    //             freight::Project::load_existing(std::filesystem::current_path());
    //         if (!project.has_value())
    //         {
    //             return EXIT_FAILURE;
    //         }

    //         if (!project->build())
    //         {
    //             return EXIT_FAILURE;
    //         }
    //     }
    //     else if (args[1] == "run")
    //     {
    //         if (args.size() > 2)
    //         {
    //             std::cerr << "Too many arguments";
    //             return EXIT_FAILURE;
    //         }

    //         auto project =
    //             freight::Project::load_existing(std::filesystem::current_path());
    //         if (!project.has_value())
    //         {
    //             return EXIT_FAILURE;
    //         }

    //         if (!project->run())
    //         {
    //             return EXIT_FAILURE;
    //         }
    //     }
    // }

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) { return EXIT_FAILURE; }