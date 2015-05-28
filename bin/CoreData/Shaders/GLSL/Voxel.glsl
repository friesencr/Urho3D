#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "Lighting.glsl"
#include "Fog.glsl"

uniform sampler2DArray sDiffArray0;
uniform usamplerBuffer sFaceArray6;
uniform vec3 cTransform[3];
uniform vec3 cAmbientTable[4];
uniform vec3 cNormalTable[32];
// uniform vec4 cColorTable[64];
uniform vec4 cTexScale[64];
uniform vec3 cTexGen[64];

flat varying uvec4  vFacedata;
varying  vec3  vVoxelSpacePos;
varying  vec3  vNormal;
varying float  vTexLerp;
varying float  vAmbOcc;
varying vec4 vWorldPos;
varying vec3 vVertexLight;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    int faceID = gl_VertexID >> 2;
    vFacedata   = texelFetch(sFaceArray6, faceID);

    // extract data for vertex
    vec3 offset;
    offset.x = float(iData & 127u);
    offset.y = float((iData >> 14u) & 511u);
    offset.z = float((iData >> 7u) & 127u);
    vAmbOcc  = float( (iData >> 23u) &  63u ) / 63.0;
    vTexLerp = float( (iData >> 29u)        ) /  7.0;
    vNormal = cNormalTable[(vFacedata.w>>2u) & 31u];
    vVoxelSpacePos = offset * cTransform[0];
    vec3 worldPos = (vec4(vVoxelSpacePos, 1.0) * modelMatrix).xyz;
    gl_Position = GetClipPos(worldPos);

    vWorldPos = vec4(worldPos, GetDepth(gl_Position));
    vVertexLight = GetAmbient(GetZonePos(worldPos));

    #ifdef NUMVERTEXLIGHTS
        for (int i = 0; i < NUMVERTEXLIGHTS; ++i)
            vVertexLight += GetVertexLight(i, worldPos, vNormal) * cVertexLights[i * 3].rgb;
    #endif
}

void PS()
{
    // float fragment_alpha;
    vec3 normal = vNormal;

    uint tex1ID = vFacedata.x;
    uint tex2ID = vFacedata.y;
    uint texProjID = vFacedata.w & 31u;
    uint colorId  = vFacedata.z;

    vec3 texgenS = cTexGen[texProjID];
    vec3 texgenT = cTexGen[texProjID+32u];
    float tex1Scale = cTexScale[tex1ID & 63u].x;
    // vec4 color = cColorTable[colorId & 63u];

    vec2 texcoord;
    vec3 textureSpacePos = vVoxelSpacePos.xzy;
    texcoord.s = dot(textureSpacePos, texgenS);
    texcoord.t = dot(textureSpacePos, texgenT);
    vec2 texcoord1 = tex1Scale * texcoord;
    vec4 diffTex1 = texture(sDiffArray0, vec3(texcoord1, float(tex1ID)));
    // vec4 tex1 = texture(tex_array[0], vec3(texcoord_1, float(tex1_id)));\n"
    // vec4 diffTex1 = texture(sDiffuse1, vec3(texcoord1, 1.0));
    // vec4 diffInput = vec4(normalize(vWorldPos.xyz), 1.0); // diffTex1.xyz;
    vec4 diffInput = diffTex1; // diffTex1.xyz;
    // vec4 diffColor = cMatDiffColor * diffInput;
    vec4 diffColor = diffInput;
    vec3 finalColor = vVertexLight * diffColor.rgb;
    vec3 ambientColor = dot(normal, cAmbientTable[0].xyz) * cAmbientTable[1].xyz + cAmbientTable[2].xyz;
    finalColor += ambientColor * vAmbOcc * diffColor.rgb;

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(vWorldPos.w, vWorldPos.y);
    #else
        float fogFactor = GetFogFactor(vWorldPos.w);
    #endif

    // gl_FragColor = vec4(GetFog(finalColor, fogFactor), diffColor.a);
    // gl_FragColor = vec4(1.0, 0.5, 0.5, 1.0);
    gl_FragColor = vec4(GetFog(finalColor, fogFactor), diffColor.a);
    // gl_FragColor = vec4(normalize(vec4(vFacedata)).xyz, 1.0);
}
