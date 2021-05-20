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

    return Material(alb*alb, rough, met, occ, false, false);
}

Material createHardMaterial(vec3 alb, float roughness, float metal) {
    return Material(alb, roughness, metal, 1, false, false);
}

Material createEmissionMaterial(vec3 alb) {
    return Material(alb, 0, 0, 0, false, true);
}

Material getMaterial(vec3 p, inout vec3 n, int mid);
SubSurfaceMaterial getSubsurfaceMaterial(Material m, int mid);
// ==================== END MATERIAL FUNCTIONS ==========================