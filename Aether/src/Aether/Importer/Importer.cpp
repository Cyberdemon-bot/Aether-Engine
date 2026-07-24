#include "Aether/Importer/Importer.h"

namespace Aether {
    Scope<GLBAssembler> Importer::s_GLBAssembler = GLBAssembler::Create();
}