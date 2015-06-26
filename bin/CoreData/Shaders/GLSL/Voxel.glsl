#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Lighting.glsl"
#include "Fog.glsl"

uniform sampler2DArray sDiffArray0;
uniform sampler2DArray sDiffArray1;
uniform usamplerBuffer sFaceArray6;
uniform vec3 cTransform[3];
uniform vec3 cAmbientTable[4];
uniform vec3 cNormalTable[32];
uniform vec4 cColorTable[64];
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
    vec4 diffColor;
    float fragmentAlpha;
    vec3 normal = vNormal;

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
    vec2 texCoord;
    texCoord.s = dot(textureSpacePos, texgenS);
    texCoord.t = dot(textureSpacePos, texgenT);
    vec2 texCoord1 = tex1Scale * texCoord;
    vec4 diffTex1 = texture(sDiffArray0, vec3(texCoord1, float(tex1ID)));
    fragmentAlpha = diffTex1.a;

    // texture 2
    vec4 tex2Props = cTexScale[tex2ID & 63u];
    float tex2Scale = tex2Props.y;
    bool texBlendMode = tex2Props.z != 0.0;
    vec2 texCoord2 = tex2Scale * texCoord;
    vec4 diffTex2 = texture(sDiffArray0, vec3(texCoord2, float(tex2ID)));

    // voxel color
    vec4 color = cColorTable[colorId & 63u];
    if ((colorId & 64u) != 0u) {
        diffTex1.rgba *= color.rgba;
    }
    if ((colorId & 128u) != 0u) {
        diffTex2.rgba *= color.rgba;
    }

    diffTex2.a *= vTexLerp;

    if (texBlendMode)
        diffColor.rgb = cMatDiffColor.rgb * diffTex1.rgb * mix(vec3(1.0, 1.0, 1.0), 2.0 * diffTex2.xyz, diffTex2.a);
    else {
        diffColor.rgb = cMatDiffColor.rgb * mix(diffTex1.xyz, diffTex2.xyz, 0.5);
        fragmentAlpha = diffTex1.a * (1 - diffTex2.a) + diffTex2.a;
    }

    // diffColor.rgb = cMatDiffColor.rgb * diffTex1.rgb;
    
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
        // Voxel ambient light
        vec3 voxAmbientColor = dot(normal, cAmbientTable[0].xyz) * cAmbientTable[1].xyz + cAmbientTable[2].xyz;
        voxAmbientColor = clamp(voxAmbientColor, 0.0, 1.0);
        //voxAmbientColor *= (1 - vAmbOcc*2);
        //vec3 voxLitColor = diffColor.xyz * (1.0 - vAmbOcc*2);

        //vec3 voxLitColor = ComputeLighting(vWorldPos.xyz, vNormal, diffColor.xyz, voxAmbientColor);

        // Ambient & per-vertex lighting
        // vec3 finalColor = vVertexLight * vAmbOcc * diffColor.rgb;
        vec3 finalColor = vVertexLight * vAmbOcc * (diffColor.rgb + voxAmbientColor);

        //vec3 finalColor = vec3(1.0, 1.0, 1.0) * (1 - vAmbOcc);

        // gl_FragColor = vec4(0.0, 0.0, 0.0, fragmentAlpha * diffColor.a);
        gl_FragColor = vec4(GetFog(finalColor, fogFactor), fragmentAlpha * diffColor.a);
        //gl_FragColor = vec4(voxLitColor, diffColor.a);
    #endif

}
