#include "pch.h"

#include "cli.h"

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

int main(int argc, char **argv) {
	Cli cli;
	cli.start(argc, argv);

	return EXIT_FAILURE;
}