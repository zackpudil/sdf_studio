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
    bool emmissive;

    bool trasmit;
    float transmitAmount;
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
uniform float envExp;
uniform int useIrr;

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

vec3 modifyDirectionWithRoughness( const vec3 n, const float roughness, inout float seed) {
  	vec2 r = hash2(seed);
    
	vec3  uu = normalize(cross(n, abs(n.y) > .5 ? vec3(1.,0.,0.) : vec3(0.,1.,0.)));
	vec3  vv = cross(uu, n);
	
    float a = roughness*roughness*roughness*roughness;
    
	float rz = sqrt(abs((1.0-r.y) / clamp(1.+(a - 1.)*r.y,.00001,1.)));
	float ra = sqrt(abs(1.-rz*rz));
	float rx = ra*cos(2*PI*r.x); 
	float ry = ra*sin(2*PI*r.x);
	vec3  rr = vec3( rx*uu + ry*vv + rz*n );
    
    return normalize(rr);
}

vec2 randomInUnitDisk(inout float seed) {
    vec2 h = hash2(seed) * vec2(1.,2*PI);
    float phi = h.y;
    float r = sqrt(h.x);
	return r*vec2(sin(phi),cos(phi));
}

float ggx(float nov, float r) {
    float k = pow(r + 1.0, 2.0)/8.0;
    return nov/(nov*(1.0 - k) + k);
}

vec3 diffuseCoverage(vec3 nor, vec3 rd, vec3 l, Material mat) {
    vec3 v = normalize(-rd);
    vec3 h = normalize(v + l);
    float hov = max(dot(h, v), 0);
    float nol = max(dot(nor, l), 0);

    vec3 f0 = mix(vec3(0.04), mat.albedo, mat.metal);
    vec3 F = f0 + (1.0 - f0)*pow(max(1 - hov, 0), 5);

    return (1.0 - F)*(1.0 - mat.metal)*mat.albedo*nol/PI;

}

vec3 specularCoverage(vec3 nor, vec3 rd, vec3 l, Material mat) {
    vec3 v = normalize(-rd);
    vec3 h = normalize(v + l);

    float hov = max(dot(h, v), 0);

    vec3 f0 = mix(vec3(0.04), mat.albedo, mat.metal);
    vec3 F = f0 + (1.0 - f0)*pow(max(1 - hov, 0), 5);

    float r = max(0.001, mat.roughness);
    float a = r*r;
    float a2 = a*a;
    float noh = max(dot(nor, h), 0.0);
    float noh2 = noh*noh;

    float D = a2/(pow(noh2*(a2 - 1.0) + 1.0, 2.0)*PI);

    float nov = max(dot(nor, v), 0);
    float nol = max(dot(nor, l), 0);
    
    float G = ggx(nov, mat.roughness)*ggx(nol, mat.roughness);

    float denom = 4.0*nov*nol;
    return F*D*G*nol/max(denom, 0.001);
}

// =================== END LIGHT TRACING BRDF FUNCTIONS ========================
// TODO: This should be a uniform.
#define PATH_LENGTH 9

vec3 sdfs_pathtrace(vec3 ro, vec3 rd, inout float seed) {
    vec3 sig = vec3(1);
    vec3 col = vec3(0);
    bool isBackground = true;

    for(int bounce = 0; bounce < PATH_LENGTH; bounce++) {
        int mid = 0;
        float dist = sdfs_trace(ro, rd, maxDistance, mid);

        if(dist < maxDistance) {
            isBackground = false;
            vec3 pos = ro + rd*dist;
            vec3 nor = sdfs_getNormal(pos);
            Material mat = getMaterial(pos, nor, mid);

            if (mat.emmissive) {
                col += sig*mat.albedo;
                return col;
            }

            vec3 f0 = mix(vec3(0.04), mat.albedo, mat.metal);

            for(int i = 0; i < numberOfLights; i++) {
                if(i >= numberOfLights) break;
                Light light = lights[i];

                vec3 lightDirection = light.type == 0
                    ? normalize(light.position)
                    : normalize(light.position - pos);

                float lightDist = light.type == 0
                    ? maxDistance
                    : length(light.position - pos);

                // determine how we handle this light, either specular or diffuse based coverage
                float F = FresnelSchlickRoughness(max(0.0, -dot(nor, lightDirection)), 0.04, mat.roughness);
                if (F > hash1(seed) - mat.metal) {
                    vec3 coverage = clamp(specularCoverage(nor, rd, lightDirection, mat), 0, 1);
                    col += sig*coverage*light.color;
                } else {
                    vec3 coverage = diffuseCoverage(nor, rd, lightDirection, mat);
                    // we want the shadow for a glass to be proportional to n*(-l) * lightDirection,
                    // so we check what the shadow ray hits to see if it's a glass (transmit) material
                    vec3 sha;

                    int hitId;
                    float hitDist = sdfs_trace(pos+nor*0.01, lightDirection, lightDist, hitId);
                    if (hitDist < lightDist) {
                        vec3 hitPos = pos+nor*0.01 + lightDirection*hitDist;
                        vec3 hitNor = sdfs_getNormal(hitPos);
                        Material hitM = getMaterial(hitPos, hitNor, hitId);

                        if (hitM.trasmit) {
                            sha = clamp(dot(-lightDirection, hitNor) - pow(hitM.roughness, 4), 0, 1)*hitM.albedo;
                        } else {
                            sha = vec3(0.0);
                        }
                    } else {
                        sha = vec3(1);
                    }
                    col += sig*coverage*light.color*sha;
                }
            }

            ro = pos;
            if (mat.trasmit) {
                //sig *= mat.albedo*mat.ambientOcclusion;
                float F = FresnelSchlickRoughness(max(0, dot(-nor, rd)), pow(mat.roughness, 4), 0);
                vec3 wo;
                if (F < hash1(seed)) {
                    wo = modifyDirectionWithRoughness(refract(rd, nor, 1 / (1.0 + mat.transmitAmount)), pow(mat.roughness, 4), seed);
                    ro += 2*max(0.01, abs(sdfs_getGeometry(ro + wo*0.01)))*wo;
                    sig *= mat.albedo;
                } else {
                    wo = modifyDirectionWithRoughness(reflect(rd, nor), mat.roughness, seed);
                    sig *= clamp(specularCoverage(nor, rd, wo, mat), 0, 1);
                }
                rd = wo;
                continue;
            }

            // if we have metal, do a random reflect in a cone proportional to it's roughness,
            // the signal is also updated.
            float F = FresnelSchlickRoughness(max(0.0, -dot(nor, rd)), 0.04, mat.roughness);
            if (mat.metal >= hash1(seed) || F > hash1(seed)) {
                vec3 wo = modifyDirectionWithRoughness(reflect(rd, nor), mat.roughness, seed);
                sig *= clamp(specularCoverage(nor, rd, wo, mat), 0, 1);
                sig *= mix(vec3(1), mat.albedo, mat.metal);
                rd = wo;
                continue;
            } 

            // if we get here, that means we have a simple diffuse brdf to handle.
            vec3 wo = cosWeightedRandomHemisphereDirection(nor, seed);
            sig *= diffuseCoverage(nor, rd, wo, mat);
            rd = wo;
        } else {
            if (hasEnvMap == 1) {
                if (isBackground) {
                    return useIrr == 1
                        ? texture(irr, rd).rgb
                        : textureLod(prefilter, rd, 0).rgb;
                }
                col += sig*(1.0 - exp(-envExp*textureLod(prefilter, rd, 0).rgb));
            }
            return col;
        }
    }

    return col;
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
