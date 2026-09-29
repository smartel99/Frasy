// Portable wkhtmltopdf: a minimal command-line front-end over wkhtmltox.dll.
// Ship this executable alongside wkhtmltox.dll; no installer required.

#include <wkhtmltox/pdf.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace {
bool g_quiet    = false;
bool g_hadError = false;

void onError(wkhtmltopdf_converter*, const char* msg)
{
    g_hadError = true;
    std::fprintf(stderr, "Error: %s\n", msg);
}

void onWarning(wkhtmltopdf_converter*, const char* msg)
{
    if (!g_quiet) { std::fprintf(stderr, "Warning: %s\n", msg); }
}

void onPhaseChanged(wkhtmltopdf_converter* c)
{
    if (g_quiet) { return; }
    int phase = wkhtmltopdf_current_phase(c);
    std::fprintf(stderr,
                 "[%d/%d] %s\n",
                 phase + 1,
                 wkhtmltopdf_phase_count(c),
                 wkhtmltopdf_phase_description(c, phase));
}

void printUsage(const char* exe)
{
    std::fprintf(stderr,
                 "Usage: %s [options] <input.html|url> <output.pdf>\n"
                 "\n"
                 "Options:\n"
                 "  -q, --quiet                    Suppress progress and warnings\n"
                 "  -s, --page-size <size>         Page size (A4, Letter, ...). Default: A4\n"
                 "  -O, --orientation <o>          Portrait or Landscape\n"
                 "  -T, --margin-top <unit>        Top margin (e.g. 10mm)\n"
                 "  -B, --margin-bottom <unit>     Bottom margin\n"
                 "  -L, --margin-left <unit>       Left margin\n"
                 "  -R, --margin-right <unit>      Right margin\n"
                 "  -d, --dpi <dpi>                Output DPI\n"
                 "  -g, --grayscale                Generate a grayscale PDF\n"
                 "      --title <text>            PDF document title\n"
                 "      --print-media-type        Use the print media-type instead of screen\n"
                 "      --no-background           Do not print the page background\n"
                 "      --disable-javascript      Do not run JavaScript\n"
                 "      --javascript-delay <ms>   Wait for JavaScript to finish (default 200)\n"
                 "      --enable-local-file-access   Allow access to local files\n"
                 "      --disable-local-file-access  Block access to local files\n"
                 "      --global <name>=<value>   Set any raw wkhtmltopdf global setting\n"
                 "      --object <name>=<value>   Set any raw wkhtmltopdf object setting\n"
                 "  -V, --version                  Print the wkhtmltox version\n"
                 "  -h, --help                     Show this help\n",
                 exe);
}

bool splitKeyValue(const std::string& arg, std::pair<std::string, std::string>& out)
{
    auto eq = arg.find('=');
    if (eq == std::string::npos || eq == 0) { return false; }
    out = {arg.substr(0, eq), arg.substr(eq + 1)};
    return true;
}
}    // namespace

int main(int argc, char** argv)
{
    using Setting = std::pair<std::string, std::string>;
    std::vector<Setting>     globals;
    std::vector<Setting>     objects;
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto next = [&](const char* opt) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Missing value for option %s\n", opt);
                std::exit(EXIT_FAILURE);
            }
            return argv[++i];
        };

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return EXIT_SUCCESS;
        }
        else if (arg == "-V" || arg == "--version") {
            std::printf("wkhtmltox %s\n", wkhtmltopdf_version());
            return EXIT_SUCCESS;
        }
        else if (arg == "-q" || arg == "--quiet") { g_quiet = true; }
        else if (arg == "-s" || arg == "--page-size") { globals.emplace_back("size.pageSize", next(argv[i])); }
        else if (arg == "-O" || arg == "--orientation") { globals.emplace_back("orientation", next(argv[i])); }
        else if (arg == "-T" || arg == "--margin-top") { globals.emplace_back("margin.top", next(argv[i])); }
        else if (arg == "-B" || arg == "--margin-bottom") { globals.emplace_back("margin.bottom", next(argv[i])); }
        else if (arg == "-L" || arg == "--margin-left") { globals.emplace_back("margin.left", next(argv[i])); }
        else if (arg == "-R" || arg == "--margin-right") { globals.emplace_back("margin.right", next(argv[i])); }
        else if (arg == "-d" || arg == "--dpi") { globals.emplace_back("dpi", next(argv[i])); }
        else if (arg == "-g" || arg == "--grayscale") { globals.emplace_back("colorMode", "Grayscale"); }
        else if (arg == "--title") { globals.emplace_back("documentTitle", next(argv[i])); }
        else if (arg == "--print-media-type") { objects.emplace_back("web.printMediaType", "true"); }
        else if (arg == "--no-background") { objects.emplace_back("web.background", "false"); }
        else if (arg == "--disable-javascript") { objects.emplace_back("web.enableJavascript", "false"); }
        else if (arg == "--javascript-delay") { objects.emplace_back("load.jsdelay", next(argv[i])); }
        else if (arg == "--enable-local-file-access") { objects.emplace_back("load.blockLocalFileAccess", "false"); }
        else if (arg == "--disable-local-file-access") { objects.emplace_back("load.blockLocalFileAccess", "true"); }
        else if (arg == "--global" || arg == "--object") {
            const char* opt = argv[i];
            Setting     kv;
            if (!splitKeyValue(next(opt), kv)) {
                std::fprintf(stderr, "Expected <name>=<value> after %s\n", opt);
                return EXIT_FAILURE;
            }
            (arg == "--global" ? globals : objects).push_back(std::move(kv));
        }
        else if (arg.size() > 1 && arg[0] == '-') {
            std::fprintf(stderr, "Unknown option: %s\n\n", arg.c_str());
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }
        else { positional.push_back(std::move(arg)); }
    }

    if (positional.size() != 2) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }
    const std::string& input  = positional[0];
    const std::string& output = positional[1];

    if (wkhtmltopdf_init(0) != 1) {
        std::fprintf(stderr, "Failed to initialize wkhtmltopdf\n");
        return EXIT_FAILURE;
    }

    wkhtmltopdf_global_settings* gs = wkhtmltopdf_create_global_settings();
    wkhtmltopdf_set_global_setting(gs, "size.pageSize", "A4");
    wkhtmltopdf_set_global_setting(gs, "out", output.c_str());
    for (const auto& [name, value] : globals) {
        if (wkhtmltopdf_set_global_setting(gs, name.c_str(), value.c_str()) != 1) {
            std::fprintf(stderr, "Invalid global setting: %s=%s\n", name.c_str(), value.c_str());
        }
    }

    wkhtmltopdf_object_settings* os = wkhtmltopdf_create_object_settings();
    wkhtmltopdf_set_object_setting(os, "page", input.c_str());
    for (const auto& [name, value] : objects) {
        if (wkhtmltopdf_set_object_setting(os, name.c_str(), value.c_str()) != 1) {
            std::fprintf(stderr, "Invalid object setting: %s=%s\n", name.c_str(), value.c_str());
        }
    }

    // The converter takes ownership of the global settings and of each added object's settings.
    wkhtmltopdf_converter* converter = wkhtmltopdf_create_converter(gs);
    wkhtmltopdf_set_error_callback(converter, onError);
    wkhtmltopdf_set_warning_callback(converter, onWarning);
    wkhtmltopdf_set_phase_changed_callback(converter, onPhaseChanged);
    wkhtmltopdf_add_object(converter, os, nullptr);

    bool ok = wkhtmltopdf_convert(converter) == 1;
    if (int http = wkhtmltopdf_http_error_code(converter); http != 0) {
        std::fprintf(stderr, "HTTP error code: %d\n", http);
    }

    wkhtmltopdf_destroy_converter(converter);
    wkhtmltopdf_deinit();

    if (!ok) {
        std::fprintf(stderr, "Conversion failed%s\n", g_hadError ? "" : " (no error reported)");
        return EXIT_FAILURE;
    }
    if (!g_quiet) { std::fprintf(stderr, "Done: %s\n", output.c_str()); }
    return EXIT_SUCCESS;
}
