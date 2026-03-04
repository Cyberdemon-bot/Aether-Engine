#include "Aether/Audio/AudioSystem.h"

namespace Aether {

    Scope<AudioAPI> AudioSystem::s_AudioAPI = AudioAPI::Create();
}