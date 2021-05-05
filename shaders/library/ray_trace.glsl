// ======================== TRACING FUNCTIONS ========================
float sdfs_getGeometry(vec3 p, out int materialId) {
    return de(p, materialId);
}

float sdfs_getGeometry(vec3 p) {
    int tmp;
    return de(p, tmp);
}

int SDFS_TRACE_AMOUNT = 0;

float sdfs_trace(vec3 ro, vec3 rd, float mx, out int materialId) {
    float totalDistance = 0.0;

    float omega = 1.2;
    float candidateError = INFINITY;
    float candidateT = 0.0;
    float previousRadius = 0.0;
    float stepLength = 0.0;
    float functionSign = sdfs_getGeometry(ro) < 0 ? -1 : 1;
    
    for(int i = 0; i < 500; i++) {
        if(i >= maxIterations) break;

        float signedRadius = functionSign*sdfs_getGeometry(ro + rd*(totalDistance), materialId);
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

float sdfs_trace(vec3 ro, vec3 rd, float mx) {
    int tmp;
    return sdfs_trace(ro, rd, mx, tmp);
}

vec3 sdfs_getNormal(vec3 p) {
    vec2 h = vec2(0.001, 0.0);
    vec3 n = vec3(
        sdfs_getGeometry(p + h.xyy) - sdfs_getGeometry(p - h.xyy),
        sdfs_getGeometry(p + h.yxy) - sdfs_getGeometry(p - h.yxy),
        sdfs_getGeometry(p + h.yyx) - sdfs_getGeometry(p - h.yyx)
    );

    return normalize(n);
}

float sdfs_getAmbientOcclusion(vec3 p, vec3 n) {
    float o = 0.0, s = 0.005, w = 1.0;

    for(int i = 0; i < 10; i++) {
        float d = sdfs_getGeometry(p + n*s);
        o += (s - d)*w;
        w *= 0.98;
        s += s/float(i + 1);
    }

    return clamp(1.0 - o, 0.0, 1.0);
}

float sdfs_getSoftShadow( in vec3 ro, in vec3 rd, float mint, float maxt, float k ) {
    float res = 1.0;
    float t = mint;

    float ph = 0.0;

    for(int i = 0; i < 200; i++) {
        float h = sdfs_getGeometry(ro + rd*t);
        float y = (i == 0) ? 0.0 : h*h/(2.0*ph);
        float d = sqrt(h*h - y*y);
        res = min(res, k*d/max(0.0, t-y));
        ph = h;

        t += h*fudge;
        if(res < 0.0001 || t > maxt) break;
    }

    return clamp(res, 0.0, 1.0);
}

float sdfs_getSubsurfaceScatter(vec3 p, vec3 n, float depth) {
    float t = 0.0;

    for(int i = 0; i < 64; i++) {
        float len = rand(float(i))*depth;
        vec3 sampleDir = normalize(hash33(-n + i));
        sampleDir = sampleDir - 2*n*max(0, -dot(-n,sampleDir));

        t += len + sdfs_getGeometry(p + (sampleDir*len));
    }

    return clamp(t/64, 0.0, 1.0);
}
// ======================== END TRACING FUNCTIONS ========================