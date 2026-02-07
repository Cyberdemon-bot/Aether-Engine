#include "Aether/Importer/Importer.h"

namespace Aether {
    Scope<ImporterAPI> Importer::s_ImporterAPI = ImporterAPI::Create();
}