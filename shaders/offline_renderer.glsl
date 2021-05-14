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

uniform float fudge;
uniform float maxDistance;
uniform int maxIterations;

uniform samplerCube irr;
uniform samplerCube prefilter;
uniform int hasEnvMap;

uniform Light lights[10];
uniform int numberOfLights;

uniform sampler2D lastPass;
uniform float time;
uniform float dof;
uniform int shouldReset;

<<TEXTURES>>

<<SDF_HELPERS>>

<<NOISE>>

float de(vec3 p, out int mid);

<<RAY_TRACE>>

<<MATERIALS>>

<<LIGHTING>>

// =================== LIGHT TRACING BRDF FUNCTIONS ========================
float FresnelSchlickRoughness(float cosTheta, float F0, float roughness) {
    return F0 + (max((1. - roughness), F0) - F0) * pow(abs(1. - cosTheta), 5.0);
}

vec3 cosWeightedRandomHemisphereDirection( const vec3 n, inout float seed ) {
  	vec2 r = hash2(seed);
    
	vec3  uu = normalize(cross(n, abs(n.y) > .5 ? vec3(1.,0.,0.) : vec3(0.,1.,0.)));
	vec3  vv = cross(uu, n);
	
	float ra = sqrt(r.y);
	float rx = ra*cos(6.2831*r.x); 
	float ry = ra*sin(6.2831*r.x);
	float rz = sqrt( abs(1.0-r.y) );
	vec3  rr = vec3( rx*uu + ry*vv + rz*n );
    
    return normalize(rr);
}

vec3 modifyDirectionWithRoughness( const vec3 n, const float roughness, inout float seed, float metal ) {
  	vec2 r = hash2(seed);
    
	vec3  uu = normalize(cross(n, abs(n.y) > .5 ? vec3(1.,0.,0.) : vec3(0.,1.,0.)));
	vec3  vv = cross(uu, n);
	
    float a = roughness*roughness*roughness*roughness*roughness;
    
	float rz = sqrt(abs((1.0-r.y) / clamp(1.+(a - 1.)*r.y,.00001,1.)));
	float ra = sqrt(abs(1.-rz*rz));
	float rx = ra*cos(6.2831*r.x); 
	float ry = ra*sin(6.2831*r.x);
	vec3  rr = vec3( rx*uu + ry*vv + rz*n );
    
    return normalize(rr);
}

vec2 randomInUnitDisk(inout float seed) {
    vec2 h = hash2(seed) * vec2(1.,6.28318530718);
    float phi = h.y;
    float r = sqrt(h.x);
	return r*vec2(sin(phi),cos(phi));
}

// =================== END LIGHT TRACING BRDF FUNCTIONS ========================
#define PATH_LENGTH 5

vec3 sdfs_pathtrace(vec3 ro, vec3 rd, inout float seed) {
    vec3 col = vec3(1);
    float lastRoughness = 0.0;
    bool isBackground = true;

    for(int i = 0; i < PATH_LENGTH; i++) {
        int mid = 0;
        float dist = sdfs_trace(ro, rd, maxDistance, mid);

        if(dist < maxDistance) {
            isBackground = false;

            vec3 pos = ro + rd*dist;
            vec3 nor = sdfs_getNormal(pos);

            Material mat = getMaterial(pos, nor, mid);
            lastRoughness = mat.roughness;

            vec3 lightColor = vec3(1);

            for(int i = 0; i < 10; i++) {
                if (i >= numberOfLights) break;

                vec3 lightDirection = lights[i].type == 0
                    ? normalize(lights[i].position)
                    : normalize(lights[i].position - pos);

                float sha = step(INFINITY, sdfs_trace(pos+nor*0.005, lightDirection, maxDistance));

                lightColor += clamp(sdfs_getDirectLighting(
                    nor, lightDirection, rd, mat, sha, lights[i].color
                ), 0 ,1);
            }

            ro = pos;
            float F = FresnelSchlickRoughness(max(0.0, -dot(nor, rd)), 0.04, mat.roughness);
            // set up ray to bounce from for next calculation.
            if (F  > hash1(seed) - mat.metal) {
                if ( mat.metal > 0.5) {
                    col *= max(mat.albedo, 0.04)*lightColor;
                } else {
                   col *= lightColor;
                }
                rd = modifyDirectionWithRoughness(reflect(rd, nor), mat.roughness, seed, mat.metal);
                if (dot(rd, nor) <= 0.0) {
                    rd = cosWeightedRandomHemisphereDirection(nor, seed);
                }
            } else {
                 col *= mat.albedo*lightColor;
                rd = cosWeightedRandomHemisphereDirection(nor, seed);
            }
        } else {
            if (hasEnvMap == 1) {
                //if (isBackground) return texture(prefilter, rd).rgb;
                col *= textureLod(prefilter, rd, 4.0*lastRoughness).rgb;
            } else {
                if (isBackground) return vec3(0);
            }
            return col;
        }
    }

    return vec3(0);
}

mat3 setCamera( in vec3 ro, in vec3 ta ) {
	vec3 cw = normalize(ta-ro);
	vec3 cp = vec3(0.0, 1.0,0.0);
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

<<USER_CODE>>

void main() {
    vec2 fragCoord = gl_FragCoord.xy/resolution;
    vec2 uv = (2.0*gl_FragCoord.xy - resolution)/resolution.y;

    float seed = float(baseHash(floatBitsToUint(uv)))/float(0xffffffffU) + rand(time);

    uv += 2.0*hash2(seed)/resolution.y;
    vec3 rd = camera*normalize(vec3(uv, fov));

    float focusPlane = texture(lastPass, vec2(0)).r;
    if(all(equal(ivec2(gl_FragCoord.xy), ivec2(0)))) {
        // Calculate focus plane and store distance.
        mat3 cam = setCamera(eye, vec3(0));
        float nfpd = sdfs_trace(eye, normalize(cam*vec3(0, 0, fov)), maxDistance);
		out_fragColor = vec4(vec3(nfpd), 1);
        return;
    }

    vec3 fp = eye + rd*focusPlane;
    vec3 ro = eye + camera*vec3(randomInUnitDisk(seed), 0)*dof;
    rd = normalize(fp - ro);

    vec4 col = vec4(sdfs_pathtrace(ro, rd, seed), 1);

    if(shouldReset == 0)
        col += texture(lastPass, fragCoord);
    
    out_fragColor = col;
}
