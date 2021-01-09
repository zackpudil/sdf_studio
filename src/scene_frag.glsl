#version 330 core

#define PI acos(-1.)

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

struct Geometry {
    int materialId;
    float dist;
};

struct Material {
    vec3 albedo;

    float roughness;
    float metal;
    vec3 fresnel;
    float ambientOcclusion;
};

struct PBRTexture {
    sampler2D albedo;
    sampler2D roughness;
    sampler2D metal;
    sampler2D normal;
    sampler2D ambientOcclusion;
    sampler2D height;
};
//========================= END Type Definitions =======================

uniform vec2 resolution;
uniform mat3 camera;
uniform vec3 eye;
uniform float fov;
uniform float exposure;
uniform float fudge;

uniform samplerCube irr;
uniform samplerCube prefilter;
uniform sampler2D brdf;

uniform Light lights[10];
uniform int numberOfLights;

<<TEXTURES>>

// ========================= Random/Noise =========================
float rand(float n){return fract(sin(n) * 43758.5453123);}

float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(float p){
	float fl = floor(p);
    float fc = fract(p);
	return mix(rand(fl), rand(fl + 1.0), fc);
}
	
float noise(vec2 n) {
	const vec2 d = vec2(0.0, 1.0);
    vec2 b = floor(n), f = smoothstep(vec2(0.0), vec2(1.0), fract(n));
	return mix(mix(rand(b), rand(b + d.yx), f.x), mix(rand(b + d.xy), rand(b + d.yy), f.x), f.y);
}

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x); }

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

#define NUM_OCTAVES 5

float fbm(float x) {
	float v = 0.0;
	float a = 0.5;
	float shift = float(100);
	for (int i = 0; i < NUM_OCTAVES; ++i) {
		v += a * noise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}


float fbm(vec2 x) {
	float v = 0.0;
	float a = 0.5;
	vec2 shift = vec2(100);
	// Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
	for (int i = 0; i < NUM_OCTAVES; ++i) {
		v += a * noise(x);
		x = rot * x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}


float fbm(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(100);
	for (int i = 0; i < NUM_OCTAVES; ++i) {
		v += a * noise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

// ====================== END Randome/Noise ================================

// ======================= SDF Helpers ==========================

Geometry opCombine(Geometry a, Geometry b) {
    return a.dist < b.dist ? a : b;
}

// ====================== END SDF Helpers =======================

Geometry de(vec3 p);

Geometry sdfs_getGeomertry(vec3 p) {
    return de(p);
}

// ======================== TRACING FUNCTIONS ========================
Geometry sdfs_trace(vec3 ro, vec3 rd, float mx) {
    float totalDistance = 0.0;
    int materialId = -1;

    for(int i = 0; i < 300; i++) {
        Geometry d = sdfs_getGeomertry(ro + rd*totalDistance);
        if(d.dist < 0.0001*totalDistance || totalDistance >= mx) break;
        totalDistance += d.dist*fudge;
        materialId = d.materialId;
    }

    return Geometry(materialId, totalDistance);
}

vec3 sdfs_getNormal(vec3 p) {
    vec2 h = vec2(0.001, 0.0);
    vec3 n = vec3(
        sdfs_getGeomertry(p + h.xyy).dist - sdfs_getGeomertry(p - h.xyy).dist,
        sdfs_getGeomertry(p + h.yxy).dist - sdfs_getGeomertry(p - h.yxy).dist,
        sdfs_getGeomertry(p + h.yyx).dist - sdfs_getGeomertry(p - h.yyx).dist
    );

    return normalize(n);
}

float sdfs_getAmbientOcclusion(vec3 p, vec3 n) {
    float o = 0.0, s = 0.005, w = 1.0;

    for(int i = 0; i < 15; i++) {
        float d = sdfs_getGeomertry(p + n*s).dist;
        o += (s - d)*w;
        w *= 0.98;
        s += s/float(i + 1);
    }

    return clamp(1.0 - o, 0.0, 1.0);
}


float sdfs_getSoftShadow( in vec3 ro, in vec3 rd, float mint, float maxt, float k )
{
    float res = 1.0;
    for( float t=mint; t<maxt; )
    {
        float h = sdfs_getGeomertry(ro + rd*t).dist;
        if( h<0.0001 )
            return 0.0;
        res = min( res, k*h/t );
        t += h;
    }
    return res;
}
// ======================== END TRACING FUNCTIONS ========================

// ==================== LIGHTING ==================================
vec3 sdfs_getDirectLighting(vec3 n, vec3 l, vec3 rd,
         Material material, float sha, float dom, vec3 lc) {
    
    vec3 v = normalize(-rd);
    vec3 h = normalize(v + l);
    
    float nol = clamp(dot(n, l), 0.0, 1.0);
    float noh = clamp(dot(n, h), 0.0, 1.0);
    float nov = clamp(dot(n, v), 0.0, 1.0);
    float hov = dot(h, v);

    vec3 f = mix(vec3(material.fresnel), material.albedo, material.metal);
    vec3 F = f + (1.0 - f)*pow(1.0 - hov, 5.0);
    
    float a2 = pow(material.roughness, 4.0);
    float D = a2/pow(noh*noh*(a2 - 1.0) + 1.0, 2.0);
    
    float k = 0.5*pow(0.5*material.roughness + 0.5, 2.0);
    float kl = nol*(1.0 - k) + k;
    float kv = nov*(1.0 - k) + k;
    float V = 1.0/(4.0*kl*kv);
    
    vec3 spe = F*D*V;
    vec3 dif = (1.0 - F)*(1.0 - material.metal);
    
    return (dif*sha*material.albedo/3.141 + spe*dom)*nol*lc;
}

vec3 sdfs_getIndirectLighting(vec3 n, vec3 rd, vec3 rf,
        Material material,
        float occ, float dom) {
    
    vec3 v = normalize(-rd);
    float nov = clamp(dot(n, v), 0.0, 1.0);

    vec3 f = mix(material.fresnel, material.albedo, material.metal);
    vec3 F = f + (max(vec3(1.0 - material.roughness), f) - f)*pow(1.0 - nov, 5.0);

    vec3 irr = texture(irr, n).rgb;
    vec3 dif = irr*(1.0 - F)*(1.0 - material.metal)*material.albedo;

    vec2 ab = texture(brdf, vec2(nov, material.roughness)).rg;
    vec3 pc = textureLod(prefilter, rf, 4.0*material.roughness).rgb;
    vec3 spe = pc*(F*ab.x + ab.y);

    return (dif + spe*dom)*occ;
}

// ==================== END LIGHTING ==================================

// ==================== MATERIAL FUNCTIONS ===============================

vec4 textureTri(sampler2D s, vec3 p, vec3 n) {
    vec3 m = pow(abs(n), vec3(100.0));

    vec4 x = texture(s, p.yz);
    vec4 y = texture(s, p.xz);
    vec4 z = texture(s, p.xy);

    return (m.x*x + m.y*y + m.z*z)/(m.x + m.y + m.z);
}

vec3 getNormalBump(vec3 p, vec3 n, vec3 bump) {
    vec3 tangentNormal = bump*2.0 - 1.0;
    vec3 Q1 = dFdx(p);
    vec3 Q2 = dFdy(p);

    vec3 T = normalize(Q1 - Q2);
    vec3 B = -normalize(cross(n, T));
    mat3 TBN = mat3(T, B, n);

    return normalize(TBN*tangentNormal);
}

Material applyPBRTexture(vec3 position, inout vec3 normal, PBRTexture pbr) {
    vec3 alb = textureTri(pbr.albedo, position, normal).rgb;
    float rough = textureTri(pbr.roughness, position, normal).r;
    float occ = textureTri(pbr.ambientOcclusion, position, normal).r;
    float met = textureTri(pbr.metal, position, normal).r;

    vec3 nor = textureTri(pbr.normal, position, normal).rgb;
    normal = getNormalBump(position, normal, nor);

    return Material(alb*alb, rough, met, vec3(0.5), occ);
}

Material getMaterial(vec3 position, inout vec3 normal, Geometry geometry);
// ==================== END MATERIAL FUNCTIONS ==========================

// ==================== MAIN RENDER =====================================
vec3 sdfs_render(vec3 rayOrigin, vec3 rayDirection) {
    vec3 pixelColor = texture(irr, rayDirection).rgb*0.1;

    Geometry geometry = sdfs_trace(rayOrigin, rayDirection, 50.0);

    if(geometry.dist < 50.0) {
        vec3 position = rayOrigin + rayDirection*geometry.dist;
        vec3 normal = sdfs_getNormal(position);
        float ambientOcclusion = sdfs_getAmbientOcclusion(position, normal);

        Material material = getMaterial(position, normal, geometry);

        vec3 positionOffset = position + normal*0.001;
        vec3 reflectedRay = reflect(rayDirection, normal);

        float reflectanceOcclusion = 0.5 + 0.5*sdfs_getSoftShadow(
            positionOffset,
            reflectedRay,
            0.01, 10.0,
            32*(1.0 - material.roughness)
        );

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
                    positionOffset + normal*0.001,
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
                reflectanceOcclusion,
                lights[i].color
            );
        }

        pixelColor += sdfs_getIndirectLighting(
            normal,
            rayDirection,
            reflectedRay,
            material,
            ambientOcclusion,
            reflectanceOcclusion
        );
    }

    return pixelColor;
}
// ==================== END MAIN RENDER =====================================

<<HERE>>

void main() {
    vec2 uv = (2.0*gl_FragCoord.xy- resolution)/resolution.y;

    vec3 rd = normalize(camera*vec3(uv, fov));

    vec3 col = sdfs_render(eye, rd);

    col = 1.0 - exp(-exposure*col);
    col = pow(col, vec3(1.0/2.2));
    out_fragColor = vec4(col, 1);
}