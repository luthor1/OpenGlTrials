#include "ResourceManager.h"

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
    auto shader = std::make_shared<Shader>(vertexPath.c_str(), fragmentPath.c_str());
    m_Shaders[name] = shader;
    return shader;
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name) {
    if (m_Shaders.find(name) != m_Shaders.end())
        return m_Shaders[name];
    return nullptr;
}
