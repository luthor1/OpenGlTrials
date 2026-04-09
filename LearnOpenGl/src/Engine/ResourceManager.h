#pragma once
#include <map>
#include <string>
#include <memory>
#include "../Shader.h"

class ResourceManager {
public:
    static ResourceManager& Get() {
        static ResourceManager instance;
        return instance;
    }

    std::shared_ptr<Shader> LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    std::shared_ptr<Shader> GetShader(const std::string& name);

private:
    ResourceManager() = default;
    std::map<std::string, std::shared_ptr<Shader>> m_Shaders;
};
