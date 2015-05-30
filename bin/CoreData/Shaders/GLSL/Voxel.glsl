#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
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

#ifdef PERPIXEL
    #ifdef SHADOW
        varying vec4 vShadowPos[NUMCASCADES];
    #endif
    #ifdef SPOTLIGHT
        varying vec4 vSpotPos;
    #endif
    #ifdef POINTLIGHT
        varying vec3 vCubeMaskVec;
    #endif
#else
    varying vec3 vVertexLight;
    varying vec4 vScreenPos;
#endif

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
    vNormal = cNormalTable[(vFacedata.w>>2u) & 31u].xzy; // change to y up
    vVoxelSpacePos = offset * cTransform[0];
    vec3 worldPos = (vec4(vVoxelSpacePos, 1.0) * modelMatrix).xyz;
    gl_Position = GetClipPos(worldPos);
    vWorldPos = vec4(worldPos, GetDepth(gl_Position));

    #ifdef PERPIXEL
        // Per-pixel forward lighting
        vec4 projWorldPos = vec4(worldPos, 1.0);

        #ifdef SHADOW
            // Shadow projection: transform from world space to shadow space
            for (int i = 0; i < NUMCASCADES; i++)
                vShadowPos[i] = GetShadowPos(i, projWorldPos);
        #endif

        #ifdef SPOTLIGHT
            // Spotlight projection: transform from world space to projector texture coordinates
            vSpotPos =  projWorldPos * cLightMatrices[0];
        #endif
    
        #ifdef POINTLIGHT
            vCubeMaskVec = (worldPos - cLightPos.xyz) * mat3(cLightMatrices[0][0].xyz, cLightMatrices[0][1].xyz, cLightMatrices[0][2].xyz);
        #endif
    #else
        // Ambient & per-vertex lighting
        #if defined(LIGHTMAP) || defined(AO)
            // If using lightmap, disregard zone ambient light
            // If using AO, calculate ambient in the PS
            vVertexLight = vec3(0.0, 0.0, 0.0);
            vTexCoord2 = iTexCoord2;
        #else
            vVertexLight = GetAmbient(GetZonePos(worldPos));
        #endif
        
        #ifdef NUMVERTEXLIGHTS
            for (int i = 0; i < NUMVERTEXLIGHTS; ++i)
                vVertexLight += GetVertexLight(i, worldPos, vNormal) * cVertexLights[i * 3].rgb;
        #endif

        vScreenPos = GetScreenPos(gl_Position);

        #ifdef ENVCUBEMAP
            vReflectionVec = worldPos - cCameraPos;
        #endif
    #endif
}

void PS()
{
    float fragmentAlpha;
    vec3 normal = normalize(vNormal);

    // get data from textures
    uint tex1ID = vFacedata.x;
    uint tex2ID = vFacedata.y;
    uint texProjID = vFacedata.w & 31u;
    uint colorId  = vFacedata.z;

    // caclulate texture coords
    vec3 texgenS = cTexGen[texProjID];
    vec3 texgenT = cTexGen[texProjID+32u];
    float tex1Scale = cTexScale[tex1ID & 63u].x;
    vec3 textureSpacePos = vVoxelSpacePos.xzy; // change to y up

    // texture 1
    vec2 texcoord;
    texcoord.s = dot(textureSpacePos, texgenS);
    texcoord.t = dot(textureSpacePos, texgenT);
    vec2 texcoord1 = tex1Scale * texcoord;

    // diffuse color
    vec4 diffTex1 = texture(sDiffArray0, vec3(texcoord1, float(tex1ID)));
    vec4 diffColor = cMatDiffColor * diffTex1;

    fragmentAlpha = diffTex1.a;

    // voxel color
    // vec4 color = cColorTable[colorId & 63u];

    // built in lighting
    // vec3 voxLightColor;
    // vec3 voxLightDir = lightSource[0] - vWorldPos;
    // float voxLambert = dot(voxLightDir, normal) / dot(voxLightDir, voxLightDir);
    // vec3 voxDiffuse = clamp(lightSource[1] * clamp(voxelLambert, 0.0, 1.0), 0.0, 1.0);
    // vec3 voxLight = (voxDiffuse + voxAmbient) * diffTex1.rgb;

    // accumulate lighting values

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(vWorldPos.w, vWorldPos.y);
    #else
        float fogFactor = GetFogFactor(vWorldPos.w);
    #endif

    #if defined(PERPIXEL)
        // Per-pixel forward lighting
        vec3 lightColor;
        vec3 lightDir;
        vec3 finalColor;

        float diff = GetDiffuse(normal, vWorldPos.xyz, lightDir) * 2.0;

        #ifdef SHADOW
            diff *= GetShadow(vShadowPos, vWorldPos.w);
        #endif
    
        #if defined(SPOTLIGHT)
            lightColor = vSpotPos.w > 0.0 ? texture2DProj(sLightSpotMap, vSpotPos).rgb * cLightColor.rgb : vec3(0.0, 0.0, 0.0);
        #elif defined(CUBEMASK)
            lightColor = textureCube(sLightCubeMap, vCubeMaskVec).rgb * cLightColor.rgb;
        #else
            lightColor = cLightColor.rgb;
        #endif
    
        #ifdef SPECULAR
            float spec = GetSpecular(normal, cCameraPosPS - vWorldPos.xyz, lightDir, cMatSpecColor.a);
            finalColor = diff * lightColor * (diffColor.rgb + spec * specColor * cLightColor.a);
        #else
            finalColor = diff * lightColor * diffColor.rgb;
        #endif

        #ifdef AMBIENT
            finalColor += cAmbientColor * diffColor.rgb;
            finalColor += cMatEmissiveColor;

            // gl_FragColor = vec4(GetFog(finalColor, fogFactor), diffColor.a);
            gl_FragColor = vec4(GetFog(finalColor, fogFactor), diffColor.a);
        #else
            gl_FragColor = vec4(GetLitFog(finalColor, fogFactor), diffColor.a);
        #endif
    #else
        // Ambient & per-vertex lighting
        vec3 finalColor = vVertexLight * diffColor.rgb;

        // Voxel ambient light
        vec3 voxAmbientColor = dot(normal, cAmbientTable[0].xyz) * cAmbientTable[1].xyz + cAmbientTable[2].xyz;
        voxAmbientColor = clamp(voxAmbientColor, 0.0, 1.0);
        voxAmbientColor *= vAmbOcc;

        finalColor += voxAmbientColor * diffColor.rgb;

        // gl_FragColor = vec4(0.0, 0.0, 0.0, fragmentAlpha * diffColor.a);
        gl_FragColor = vec4(GetFog(finalColor, fogFactor), fragmentAlpha * diffColor.a);
    #endif

}
