#include <shader_loader.h>
#include <render_context.h>
#include <registry.h>

ShaderBinary ShaderLoader::Get(const std::string& path, Stage stage) {

    ShaderSource source{};
    source.name = path;
    source.stage = stage;
    source.source = context.Get<Registry>().LoadText(path.c_str());

    ShaderCompiler compiler;
    return compiler.FromSource(source);
}