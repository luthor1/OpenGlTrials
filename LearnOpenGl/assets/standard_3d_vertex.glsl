#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 uColor;
uniform vec3 uVelocity;
uniform vec3 uHolePos;

void main()
{
    // 1. Başlangıç Dünya Pozisyonu
    vec3 worldPos = vec3(model * vec4(aPos, 1.0));
    vec3 objCenter = vec3(model * vec4(0.0, 0.0, 0.0, 1.0));
    float dist = length(objCenter - uHolePos);
    
    // 2. Bükülme Şiddeti (Rs'ye yaklaştıkça parabolik artar)
    float bendFactor = pow(clamp(20.0 / max(dist, 1.0), 0.0, 1.0), 2.5);
    
    // 3. Yörüngesel Kavis (Curvature) Matematiği
    vec3 gravityDir = normalize(uHolePos - objCenter);
    vec3 velDir = length(uVelocity) > 0.001 ? normalize(uVelocity) : -gravityDir;
    
    // Sadece karadeliğe doğru değil, hızın teğet bileşenine doğru bir "muz şekli" veriyoruz
    vec3 tangent = normalize(velDir - dot(velDir, gravityDir) * gravityDir);
    
    // aPos.z bizim C++'tan gönderdiğimiz esneme (stretch) eksenimiz.
    // Kuyruk kısmının geride kalması ve kavis çizmesi için:
    vec3 curveOffset = -tangent * (aPos.z * aPos.z * bendFactor * 12.0);
    
    FragPos = worldPos + curveOffset;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = uColor;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}