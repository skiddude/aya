#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>
#include <chrono>

#include "Script/LuaVM.hpp"
#include "Utility/Utilities.hpp"

namespace fs = std::filesystem;
namespace po = boost::program_options;

bool verbose;

struct CoreScriptFile
{
    std::string name;
    fs::path file;
    bool module;
};

static int fatal(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "FATAL: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
    return 1;
}

char* hexa(unsigned char ch)
{
    static const char sym[] = "0123456789abcdef";
    static char buf[8];

    buf[0] = '0';
    buf[1] = 'x';
    buf[2] = sym[(ch >> 4) & 0xf];
    buf[3] = sym[(ch) & 0xf];
    buf[4] = 0;

    return buf;
}

void rdfile(std::string* result, const fs::path& filepath)
{
#if _MSC_VER
    FILE* vf = _wfopen(filepath.native().c_str(), L"rb");
#else
    FILE* vf = fopen(filepath.native().c_str(), "rb");
#endif

    vf || fatal("could not open '%s'\n", filepath.native().c_str());

    // no better portable way to get file size
    fseek(vf, 0, SEEK_END);
    long size = ftell(vf);
    fseek(vf, 0, SEEK_SET);

    result->resize(size);
    fread((char*)result->data(), 1, size, vf);
    fclose(vf);
}

void buildFileList(std::vector<CoreScriptFile>& files, fs::path dir, fs::path parent = fs::path())
{
    for (fs::directory_iterator it(dir), e; it != e; ++it) 
    {
        const fs::directory_entry& dirent = *it;
        const fs::path& filepath = dirent.path();
        fs::file_status st = dirent.status();

        if (st.type() == fs::file_type::directory)
        {
            // Recursively process subdirectories
            buildFileList(files, filepath, parent / filepath.filename());
        }
        else if (st.type() == fs::file_type::regular && filepath.extension() == ".lua")
        {
            fs::path name = parent / filepath.filename();

            CoreScriptFile script;
            script.name = name.replace_extension().string();

            std::replace(script.name.begin(), script.name.end(), '\\', '/');

            script.file = filepath;
            script.module = script.name.find("Modules") == 0;
            if (script.module)
                script.name = script.name.substr(8);

            files.push_back(script);

            if (verbose)
            {
                printf(" %s: %s\n", script.name.c_str(), filepath.native().c_str());
            }
        }
    }
}

void printBuffer(std::ostream& os, const std::string& name, const std::string& data)
{
    os << "static const unsigned char " << name << "[] = {";

    for (int i = 0, j = data.size(); i < j; ++i)
    {
        if (!(i % 32))
            os << "\n    ";

        os << hexa(data[i]) << ", ";
    }

    os << "\n    };\n\n";
}

void processMacro(std::string* source, const std::string& macro, const std::string& value)
{
    size_t pos = 0;
    std::string find = "${" + macro + "}";

    while ((pos = source->find(find, pos)) != std::string::npos)
    {
        source->replace(pos, find.size(), value);
        pos += value.size();
    }
}

int main(int argc, const char* argv[])
{
    // Parse arguments
    std::string outputPath;
    std::vector<std::string> sourcePaths;

    po::options_description desc("Aya.CoreScriptCompiler options");

    desc.add_options()("help,?", "Usage help")("output,o", po::value<std::string>(&outputPath)->required(),
        "Path to the output file where compiled CoreScript bytecode shall be kept")("source,s",
        po::value<std::vector<std::string>>(&sourcePaths)->required(),
        "CoreScript source paths")("verbose", po::value<bool>(&verbose)->default_value(false), "Enable verbose logging");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }

    std::vector<CoreScriptFile> files;
    fs::path source = fs::current_path() / sourcePaths[0];
    buildFileList(files, source);

    for (int i = 1; i < sourcePaths.size(); i++)
    {
        fs::path source = fs::current_path() / sourcePaths[i];
        buildFileList(files, source);
    }

    // Got all the files, go ahead and compile
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime;

    std::stringstream arrays, scripts, modules; // 3 main sections

    scripts << "static const CoreScriptBytecode gCoreScripts[] = {\n";
    modules << "static const CoreScriptBytecode gCoreModuleScripts[] = {\n";

    printf("-- Running CoreScript compiler\n");

    startTime = std::chrono::high_resolution_clock::now();

    for (unsigned i = 0; i < files.size(); ++i)
    {
        std::string source;
        CoreScriptFile& script = files[i];
        rdfile(&source, script.file);

        processMacro(&source, "PROJECT_NAME", AYA_PROJECT_NAME);
        // processMacro(&source, "CURRENCY_NAME", AYA_CURRENCY_NAME);

        std::string bytecode = LuaVM::compileCore(source);

        std::string encname = script.name;
        std::string arrayName = Aya::format("a%04u", i);
        encname = Aya::rot13(encname);

        printBuffer(arrays, arrayName, bytecode);

        (script.module ? modules : scripts) << "    { \"" << encname << "\", " << arrayName << ", " << bytecode.size() << " },\n";
    }

    endTime = std::chrono::high_resolution_clock::now();

    scripts << "};\n\n";
    modules << "};\n\n";

    auto elapsedTime = endTime - startTime;
    printf("-- Compiled %u CoreScripts in %.2f seconds\n", files.size(), std::chrono::duration<double>(elapsedTime).count());

    // Write the file
    fs::path outputFile = fs::current_path() / outputPath;
    std::ofstream output(outputFile.c_str());

    output << arrays.str() << scripts.str() << modules.str();

    if (output.rdstate() & output.failbit)
        fatal("could not write to '%s'", outputFile.c_str());

    printf("-- Built CoreScript bytecode file available at '%s'\n", outputFile.c_str());
    return 0;
}