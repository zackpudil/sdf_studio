// ==================== LIGHTING ==================================
vec3 sdfs_getDirectLighting(vec3 n, vec3 l, vec3 rd,
         Material material, float sha, vec3 lc) {
    
    vec3 v = normalize(-rd);
    vec3 h = normalize(v + l);
    
    float nol = clamp(dot(n, l), 0.0, 1.0);
    float noh = clamp(dot(n, h), 0.0, 1.0);
    float nov = clamp(dot(n, v), 0.0, 1.0);
    float hov = clamp(dot(h, v), 0.0, 1.0);

    vec3 f = mix(vec3(0.04), material.albedo, material.metal);
    vec3 F = f + max(1.0 - f, vec3(0.04))*pow(abs(1.0 - hov), 5.0);
    
    float a2 = pow(material.roughness, 4.0);
    float D = a2/pow(noh*noh*(a2 - 1.0) + 1.0, 2.0);
    
    float k = pow(material.roughness + 1, 2)/8;
    float g1 = nol/(nol*(1.0 - k) + k);
    float g2 = nov/(nov*(1.0 - k) + k);
    float G = g1*g2;
    
    vec3 spe = F*D*G/(4.0*nov*nol + 0.00001);
    vec3 dif = (1.0 - F)*(1.0 - material.metal);
    
    return (dif*sha*material.albedo/3.141 + spe)*nol*lc;
}

vec3 sdfs_computeDirectDiffuseLighting(vec3 n, vec3 rd, vec3 l, Material mat) {
    vec3 v = normalize(-rd);
    vec3 h = normalize(v + l);

    float nol = clamp(dot(n, l), 0.0, 1.0);
    float hov = dot(h, v);

    vec3 f = mix(vec3(0.04), mat.albedo, mat.metal);
    vec3 F = f + (1.0 - f)*pow(1.0 - hov, 5.0);

    vec3 dif = (1.0 - F)*(1.0 - mat.metal);
    return ((dif*mat.albedo)/PI)*nol;
}

vec3 sdfs_computeDirectSpecularLighting(vec3 n, vec3 rd, vec3 l, Material mat) {
    vec3 v = normalize(-rd);
    vec3 h = normalize(v + l);

    float nol = clamp(dot(n, l), 0.0, 1.0);
    float noh = clamp(dot(n, h), 0.0, 1.0);
    float nov = clamp(dot(n, v), 0.0, 1.0);
    float hov = clamp(dot(h, v), 0.0, 1.0);

    vec3 f = mix(vec3(0.04), mat.albedo, mat.metal);
    vec3 F = f + (1.0 - f)*pow(1.0 - hov, 5.0);

    float a2 = pow(mat.roughness, 4.0);
    float D = a2/pow(noh*noh*(a2 - 1.0) + 1.0, 2.0);

    float k = pow(mat.roughness + 1.0, 2.0)/8.0;
    float g1 = nol/nol*(1.0 - k) + k;
    float g2 = nov/nov*(1.0 - k) + k;
    float G = g1*g2;

    vec3 spe = F*D*G/(4.0*nol*nov + 0.0001);

    return spe*nol;
}

float sdfs_fresnelSchlickRoughness(float cosTheta, float F0, float roughness) {
    return F0 + (max((1. - roughness), F0) - F0) * pow(abs(1. - cosTheta), 5.0);
}

vec3 sdfs_getIndirectLighting(vec3 n, vec3 rd, vec3 rf,
        Material material,
        float occ, sampler2D brdf) {
    
    vec3 v = normalize(-rd);
    float nov = clamp(dot(n, v), 0.0, 1.0);

    vec3 f = mix(vec3(0.05), material.albedo, material.metal);
    vec3 F = f + (max(vec3(1.0 - material.roughness), f) - f)*pow(1.0 - nov, 5.0);

    vec3 i = texture(irr, n).rgb;
    vec3 dif = i*(1.0 - F)*(1.0 - material.metal)*material.albedo;

    vec2 ab = texture(brdf, vec2(nov, material.roughness)).rg;
    vec3 pc = textureLod(prefilter, rf, 4.0*material.roughness).rgb;
    vec3 spe = pc*(F*ab.x + ab.y);

    return (dif + spe)*occ;
}

// ==================== END LIGHTING ==================================
