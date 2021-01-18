#version 330 core

#define INFINITY pow(2.,8.)

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

struct Material {
    vec3 albedo;

    float roughness;
    float metal;
    vec3 fresnel;
    float ambientOcclusion;

    vec3 subsurfaceColor;
    float subsurfaceScatteringAmount;
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

float noise(vec3 x) {
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);

	float n = p.x + p.y * 157.0 + 113.0 * p.z;
	return mix(
			mix(mix(rand(n + 0.0), rand(n + 1.0), f.x),
					mix(rand(n + 157.0), rand(n + 158.0), f.x), f.y),
			mix(mix(rand(n + 113.0), rand(n + 114.0), f.x),
					mix(rand(n + 270.0), rand(n + 271.0), f.x), f.y), f.z);
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

<<SDF_HELPERS>>

float de(vec3 p, out int mId);

float sdfs_getGeomertry(vec3 p, out int materialId) {
    return de(p, materialId);
}

float sdfs_getGeomertry(vec3 p) {
    int tmp;
    return de(p, tmp);
}

// ======================== TRACING FUNCTIONS ========================
int SDFS_TRACE_AMOUNT = 0;
float sdfs_trace(vec3 ro, vec3 rd, float mx, out int materialId) {
    float totalDistance = 0.0;

    float omega = 1.2;
    float candidateError = INFINITY;
    float candidateT = 0.0;
    float previousRadius = 0.0;
    float stepLength = 0.0;
    float functionSign = sdfs_getGeomertry(ro) < 0 ? -1 : 1;
    
    for(int i = 0; i < 500; i++) {
        if(i >= maxIterations) break;

        float signedRadius = functionSign*sdfs_getGeomertry(ro + rd*(totalDistance), materialId);
        float radius = abs(signedRadius);
        bool sorFail = omega > 1 && (radius + previousRadius) < stepLength;
        if(sorFail) {
            stepLength -= omega * stepLength;
            omega = 1;
        } else {
            stepLength = signedRadius * omega;
        }

        previousRadius = radius;
        float error = radius/totalDistance;

        if(!sorFail && error < candidateError) {
            candidateT = totalDistance;
            candidateError = error;
        }

        if(!sorFail && error < 0.001 || totalDistance > maxDistance) break;
        totalDistance += (stepLength)*fudge;
        SDFS_TRACE_AMOUNT += 1;
    }

    if(totalDistance > maxDistance) return INFINITY;
    return candidateT;
}

vec3 sdfs_getNormal(vec3 p) {
    vec2 h = vec2(0.001, 0.0);
    vec3 n = vec3(
        sdfs_getGeomertry(p + h.xyy) - sdfs_getGeomertry(p - h.xyy),
        sdfs_getGeomertry(p + h.yxy) - sdfs_getGeomertry(p - h.yxy),
        sdfs_getGeomertry(p + h.yyx) - sdfs_getGeomertry(p - h.yyx)
    );

    return normalize(n);
}

float sdfs_getAmbientOcclusion(vec3 p, vec3 n) {
    float o = 0.0, s = 0.005, w = 1.0;

    int tmp;
    for(int i = 0; i < 10; i++) {
        float d = sdfs_getGeomertry(p + n*s, tmp);
        o += (s - d)*w;
        w *= 0.98;
        s += s/float(i + 1);
    }

    return clamp(1.0 - o, 0.0, 1.0);
}


float sdfs_getSoftShadow( in vec3 ro, in vec3 rd, float mint, float maxt, float k )
{
    float res = 1.0;
    int tmp;
    for( float t=mint; t<maxt; )
    {
        float h = sdfs_getGeomertry(ro + rd*t, tmp);
        if( h<0.0001 )
            return 0.0;
        res = min( res, k*h/t );
        t += h*fudge;
    }
    return res;
}

vec4 sdfs_getSubsurfaceScatter(vec3 origin, vec3 direction) {
    vec3 p = origin;
    float e = 0.0;
    for(int i = 0; i < 300; i++) {
        float d = sdfs_getGeomertry(p);
        e += -d*fudge;
        if(d > -0.001) break;
        p -= d*direction;
    }

    return vec4(p, e);
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

vec4 textureTriPlannar(sampler2D s, vec3 p, vec3 n) {
    vec3 m = pow(abs(n), vec3(100.0));

    vec4 x = texture(s, p.yz)*m.x;
    vec4 y = texture(s, p.xz)*m.y;
    vec4 z = texture(s, p.xy)*m.z;

    return (x + y + z)/(m.x + m.y + m.z);
}

#define pToUv(p, n, uv) (uv(p.yz)*(pow(abs(n.x), 10.0)) + uv(p.xz)*(pow(abs(n.y), 10.0)) + uv(p.xy)*(pow(abs(n.z), 10.0)))/(pow(abs(n.x), 10.0) + pow(abs(n.y), 10.0) + pow(abs(n.z), 10.0))
#define getSDFNormal(p, sdf) normalize(vec3(sdf(p + vec3(0.001, 0, 0)) - sdf(p - vec3(0.001, 0, 0)), sdf(p + vec3(0, 0.001, 0)) - sdf(p - vec3(0, 0.001, 0)), sdf(p + vec3(0, 0, 0.001)) - sdf(p - vec3(0, 0, 0.001))))

vec3 getNormalBump(vec3 p, vec3 n, vec3 bump) {
    vec3 tangentNormal = bump*2.0 - 1.0;

    vec3 T = normalize(vec3(0));
    vec3 B = normalize(cross(n, T));
    mat3 TBN = mat3(T, B, n);

    return normalize(TBN*tangentNormal);
}

Material applyPBRTexture(vec3 position, inout vec3 normal, PBRTexture pbr) {
    vec3 alb = textureTriPlannar(pbr.albedo, position, normal).rgb;
    float rough = textureTriPlannar(pbr.roughness, position, normal).r;
    float occ = textureTriPlannar(pbr.ambientOcclusion, position, normal).r;
    float met = textureTriPlannar(pbr.metal, position, normal).r;

    vec3 nor = textureTriPlannar(pbr.normal, position, normal).rgb;
    normal = getNormalBump(position, normal, nor);

    return Material(alb*alb, rough, met, vec3(0.05), occ, vec3(0), 0);
}

Material getMaterial(vec3 position, inout vec3 normal, int mId);
// ==================== END MATERIAL FUNCTIONS ==========================

// ===================== DEBUGGIN FUNCTIONS =============================
vec3 fusion(float x) {
	float t = clamp(x,0.0,1.0);
	return clamp(vec3(sqrt(t), t*t*t, max(sin(PI*1.75*t), pow(t, 12.0))), 0.0, 1.0);
}

vec3 fusionHDR(float x) {
	float t = clamp(x,0.0,1.0);
	return fusion(sqrt(t))*(0.5+2.*t);
}

vec3 distanceMeter(float dist, float rayLength, vec3 rayDir, float camHeight) {
    float idealGridDistance = 20.0/rayLength*pow(abs(rayDir.y),0.8);
    float nearestBase = floor(log(idealGridDistance)/log(10.));
    float relativeDist = abs(dist/camHeight);
    
    float largerDistance = pow(10.0,nearestBase+1.);
    float smallerDistance = pow(10.0,nearestBase);

   
    vec3 col = fusionHDR(log(1.+relativeDist));
    col = max(vec3(0.),col);
    if (sign(dist) < 0.) {
        col = col.grb*3.;
    }

    float l0 = (pow(0.5+0.5*cos(dist*PI*2.*smallerDistance),10.0));
    float l1 = (pow(0.5+0.5*cos(dist*PI*2.*largerDistance),10.0));
    
    float x = fract(log(idealGridDistance)/log(10.));
    l0 = mix(l0,0.,smoothstep(0.5,1.0,x));
    l1 = mix(0.,l1,smoothstep(0.0,0.5,x));

    col.rgb *= 0.1+0.9*(1.-l0)*(1.-l1);
    return col;
}
// ===================== END DEBUGGIN FUNCTIONS =========================

// ==================== MAIN RENDER =====================================
vec3 sdfs_render(vec3 rayOrigin, vec3 rayDirection) {
    vec3 pixelColor = texture(irr, rayDirection).rgb;

    int materialId;
    float geometry = sdfs_trace(rayOrigin, rayDirection, maxDistance, materialId);

    if(showRayMarchAmount == 1) {
        return mix(
            vec3(0, 0, 1),
            vec3(1, 0, 0),
            float(SDFS_TRACE_AMOUNT)/float(maxIterations));
    }

    if (useDebugPlane == 1) {
        float dt = INFINITY;
        if(rayDirection.y < 0) {
            dt = (rayOrigin.y - debugPlaneHeight)/-rayDirection.y;
        }

        if(geometry > dt) {
            return distanceMeter(sdfs_getGeomertry(rayOrigin + dt*rayDirection), dt, rayDirection, rayOrigin.y);
        }
    }

    if(geometry < maxDistance) {
        pixelColor = vec3(0);
        vec3 position = rayOrigin + rayDirection*geometry;
        vec3 normal = sdfs_getNormal(position);
        float ambientOcclusion = sdfs_getAmbientOcclusion(position, normal);

        Material material = getMaterial(position, normal, materialId);

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

            if(material.metal == 0 && material.subsurfaceScatteringAmount != 0) {
                vec3 stepOff = normalize(mix(-normal, rayDirection, 0.5));
                vec4 subsurfaceResult = sdfs_getSubsurfaceScatter(position+stepOff*0.2, rayDirection);
                float subAmount = 1.0 + 9.0*smoothstep(0.0, 1.0, 1.0-material.subsurfaceScatteringAmount);
                float subsurfaceScattering = max(0.0, (1.0 - subAmount*subsurfaceResult.w));
                float surfaceVisibility = max(0.0,
                    dot(
                        sdfs_getNormal(subsurfaceResult.xyz),
                        normalize(lightDirection - subsurfaceResult.xyz)
                    )
                );
                pixelColor += lights[i].color*material.subsurfaceColor*mix(subsurfaceScattering, surfaceVisibility, 0.2)/numberOfLights;
            }
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