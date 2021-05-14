#version 330 core

#define INFINITY pow(2.,8.)
#define sat(p) clamp(p, 0.0, 1.0)

in vec2 tex;
out vec4 out_fragColor;

//========================= Type Definitions =======================
struct Light {
    int type;
    vec3 position;
    vec3 color;
    int hasShadow;
    float shadowPenumbra;
};

struct SubSurfaceMaterial {
    vec3 albedo;
    
    float ambient;
    float depth;
    float distortion;
    float power;
};

struct Material {
    vec3 albedo;

    float roughness;
    float metal;
    float ambientOcclusion;

    bool subsurface;
};

struct PBRTexture {
    sampler2D albedo;
    sampler2D roughness;
    sampler2D metal;
    sampler2D normal;
    sampler2D ambientOcclusion;
    sampler2D height;
};
;
//========================= END Type Definitions =======================

uniform vec2 resolution;
uniform mat3 camera;
uniform vec3 eye;
uniform float fov;
uniform float exposure;


uniform float fudge;
uniform float maxDistance;
uniform int maxIterations;
uniform int useDebugPlane;
uniform float debugPlaneHeight;
uniform int showRayMarchAmount;

uniform samplerCube irr;
uniform samplerCube prefilter;
uniform sampler2D brdf;

uniform Light lights[10];
uniform int numberOfLights;

<<TEXTURES>>

<<NOISE>>

<<SDF_HELPERS>>

float de(vec3 p, out int mid);

<<RAY_TRACE>>

<<MATERIALS>>

<<DEBUG>>

<<LIGHTING>>

// ==================== MAIN RENDER =====================================
vec3 sdfs_render(vec3 rayOrigin, vec3 rayDirection) {
    vec3 pixelColor = texture(prefilter, rayDirection).rgb;

    int materialId;
    float geometry = sdfs_trace(rayOrigin, rayDirection, maxDistance, materialId);

    if (useDebugPlane == 1) {
        float dt = INFINITY;
        if(rayDirection.y < 0) {
            dt = (rayOrigin.y - debugPlaneHeight)/-rayDirection.y;
        }

        if(geometry > dt) {
            return distanceMeter(sdfs_getGeometry(rayOrigin + dt*rayDirection), dt, rayDirection, rayOrigin.y);
        }
    }

    if(showRayMarchAmount == 1) {
        return mix(
            vec3(0, 0, 1),
            vec3(1, 0, 0),
            float(SDFS_TRACE_AMOUNT)/float(maxIterations));
    }

    if(geometry < maxDistance) {
        pixelColor = vec3(0);
        vec3 position = rayOrigin + rayDirection*geometry;
        vec3 normal = sdfs_getNormal(position);
        float ambientOcclusion = sdfs_getAmbientOcclusion(position, normal);

        Material material = getMaterial(position, normal, materialId);

        vec3 reflectedRay = reflect(rayDirection, normal);

        ambientOcclusion *= material.ambientOcclusion;

        for(int i = 0; i < 10; i++) {
            if(i >= numberOfLights) break;

            vec3 lightDirection = vec3(0);

            if(lights[i].type == 0)  {
                lightDirection = normalize(lights[i].position);
            } else {
                lightDirection = normalize(lights[i].position - position);
            }

            float directLightShadow = 1.0;
            if(lights[i].hasShadow == 1) {
                directLightShadow = sdfs_getSoftShadow(
                    position + normal*0.005,
                    lightDirection,
                    0.01, 10.0,
                    lights[i].shadowPenumbra
                );
            }

            pixelColor += sdfs_getDirectLighting(
                normal,
                lightDirection,
                rayDirection,
                material,
                directLightShadow,
                lights[i].color
            );

            if(material.metal == 0 && material.subsurface) {
                SubSurfaceMaterial sssMate = getSubsurfaceMaterial(material, materialId);

                vec3 toEye = -rayDirection;
                vec3 sssLight = lightDirection + normal*sssMate.distortion;
                float sssDot = pow(sat(dot(toEye, -sssLight)), 0.1 + sssMate.power);

                float thickness = sdfs_getSubsurfaceScatter(position, normal, sssMate.depth);
                float sss = (sssDot + sssMate.ambient)*thickness;

                pixelColor += lights[i].color*sssMate.albedo*sss;
            }
        }

        pixelColor += sdfs_getIndirectLighting(
            normal,
            rayDirection,
            reflectedRay,
            material,
            ambientOcclusion,
            brdf
        );
    }

    return pixelColor;
}
// ==================== END MAIN RENDER =====================================

<<USER_CODE>>

void main() {
    vec2 uv = (2.0*gl_FragCoord.xy- resolution)/resolution.y;

    vec3 rd = normalize(camera*vec3(uv, fov));

    vec3 col = sdfs_render(eye, rd);

    col = 1.0 - exp(-exposure*col);
    col = pow(col, vec3(1.0/2.2));
    out_fragColor = vec4(col, 1);
}