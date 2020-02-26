#version 400

uniform vec2 res;

////////////////////////////////////////////////////////////////
// BOILERPLATE UTILITIES...................
const float pi = 3.14159;
const float pi2 = pi * 2.;

float opU(float d1, float d2) { return min(d1, d2); }
float opS(float d2, float d1) { return max(-d1, d2); }
float opI(float d1, float d2) { return max(d1, d2); }

mat2 rot2D(float r) {
    float c = cos(r), s = sin(r);
    return mat2(c, s, -s, c);
}
float nsin(float a) { return .5 + .5 * sin(a); }
float ncos(float a) { return .5 + .5 * cos(a); }
vec3 saturate(vec3 a) { return clamp(a, 0., 1.); }
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}
float rand(float n) { return fract(cos(n * 89.42) * 343.42); }
float dtoa(float d, float amount) {
    return clamp(1. / (clamp(d, 1. / amount, 1.) * amount), 0., 1.);
}
float sdAxisAlignedRect(vec2 uv, vec2 tl, vec2 br) {
    vec2 d = max(tl - uv, uv - br);
    return length(max(vec2(0.), d)) + min(0., max(d.x, d.y));
}
float sdCircle(vec2 uv, vec2 origin, float radius) {
    return length(uv - origin) - radius;
}
// 0-1 1-0
float smoothstep4(float e1, float e2, float e3, float e4, float val) {
    return min(smoothstep(e1, e2, val), 1. - smoothstep(e3, e4, val));
}
// hash & simplex noise from https://www.shadertoy.com/view/Msf3WH
vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1. + 2. * fract(sin(p) * 43758.5453123);
}
// returns -.5 to 1.5. i think.
float noise(in vec2 p) {
    const float K1 = .366025404;  // (sqrt(3)-1)/2;
    const float K2 = .211324865;  // (3-sqrt(3))/6;

    vec2 i = floor(p + (p.x + p.y) * K1);

    vec2 a = p - i + (i.x + i.y) * K2;
    vec2 o = (a.x > a.y) ? vec2(1., 0.)
                         : vec2(0.,
                                1.);  // vec2 of = 0.5 + 0.5*vec2(sign(a.x-a.y),
                                      // sign(a.y-a.x));
    vec2 b = a - o + K2;
    vec2 c = a - 1. + 2. * K2;

    vec3 h = max(.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.);

    vec3 n =
        h * h * h * h *
        vec3(dot(a, hash(i + 0.)), dot(b, hash(i + o)), dot(c, hash(i + 1.)));

    return dot(n, vec3(70.));
}
float noise01(vec2 p) { return clamp((noise(p) + .5) * .5, 0., 1.); }

float smoothf(float x) { return x * x * x * (x * (x * 6. - 15.) + 10.); }

////////////////////////////////////////////////////////////////
// APP CODE ...................

// this function will produce a line with brush strokes. the inputs are such
// that you can apply it to pretty much any line; the geometry is separated from
// this function.
vec3 colorBrushStroke(vec2 uvLine, vec2 uvPaper, vec2 lineSize,
                      float sdGeometry, vec3 inpColor, vec4 brushColor) {
    float posInLineY =
        (uvLine.y /
         lineSize.y);  // position along the line. in the line is 0-1.

    // brush stroke fibers effect.
    float strokeBoundary =
        dtoa(sdGeometry, 300.);  // keeps stroke texture inside the geometry.
    float strokeTexture =
        0. +
        noise01(uvLine * vec2(min(res.y, res.x) * .2, 1.))  // high freq fibers
        + noise01(uvLine * vec2(79., 1.))  // smooth brush texture. lots of room
        // for variation here, also layering.
        +
        noise01(uvLine * vec2(14., 1.))  // low freq noise, gives more variation
        ;
    strokeTexture *= .333 * strokeBoundary;  // 0 to 1 (take average of above)
    strokeTexture =
        max(.008, strokeTexture);  // avoid 0; it will be ugly to modulate
    // fade it from very dark to almost nonexistent by manipulating the curve
    // along Y
    float strokeAlpha =
        pow(strokeTexture, max(0., posInLineY) + .09);  // add allows bleeding
    // fade out the end of the stroke by shifting the noise curve below 0
    const float strokeAlphaBoost = 1.09;
    if (posInLineY > 0.)
        strokeAlpha = strokeAlphaBoost *
                      max(0., strokeAlpha - pow(posInLineY, .5));  // fade out
    else
        strokeAlpha *= strokeAlphaBoost;

    strokeAlpha = smoothf(strokeAlpha);

    // paper bleed effect.
    float paperBleedAmt =
        60. + (rand(uvPaper.y) * 30.) + (rand(uvPaper.x) * 30.);
    //    amt = 500.;// disable paper bleed

    // blotches (per stroke)
    // float blotchAmt = smoothstep(17.,18.5,magicBox(vec3(uvPaper, uvLine.x)));
    // blotchAmt *= 0.4;
    // strokeAlpha += blotchAmt;

    float alpha = strokeAlpha * brushColor.a * dtoa(sdGeometry, paperBleedAmt);
    alpha = clamp(alpha, 0., 1.);
    return mix(inpColor, brushColor.rgb, alpha);
}

vec3 colorBrushStrokeLine(vec2 uv, vec3 inpColor, vec4 brushColor, vec2 p1_,
                          vec2 p2_, float lineWidth) {
    // flatten the line to be axis-aligned.
    float lineAngle = pi - atan(p1_.x - p2_.x, p1_.y - p2_.y);
    mat2 rotMat = rot2D(lineAngle);

    float lineLength = distance(p2_, p1_);
    // make an axis-aligned line from this line.
    vec2 tl = (p1_ * rotMat);            // top left
    vec2 br = tl + vec2(0, lineLength);  // bottom right
    vec2 uvLine = uv * rotMat;

    // make line slightly narrower at end.
    lineWidth *= mix(1., .9, smoothstep(tl.y, br.y, uvLine.y));

    // wobble it around, humanize
    float res = min(res.y, res.x);
    uvLine.x += (noise01(uvLine * 1.) - .5) * .02;
    uvLine.x += cos(uvLine.y * 3.) * .009;  // smooth lp wave
    uvLine.x += (noise01(uvLine * 5.) - .5) *
                .005;  // a sort of random waviness like individual strands are
                       // moving around
    uvLine.x += (noise01(uvLine * res * 0.18) - 0.5) *
                0.0035;  // HP random noise makes it look less scientific

    // calc distance to geometry. actually just do a straight line, then we will
    // round it out to create the line width.
    float d = sdAxisAlignedRect(uvLine, tl, br) - lineWidth / 2.;
    uvLine = tl - uvLine;

    vec2 lineSize = vec2(lineWidth, lineLength);

    vec3 ret = colorBrushStroke(vec2(uvLine.x, -uvLine.y), uv, lineSize, d,
                                inpColor, brushColor);
    return ret;
}

vec2 getuv_centerX(vec2 fragCoord, vec2 newTL, vec2 newSize) {
    vec2 ret = vec2(
        fragCoord.x / res.x,
        (res.y - fragCoord.y) / res.y);  // ret is now 0-1 in both dimensions
    ret *= newSize;                      // scale up to new dimensions
    float aspect = res.x / res.y;
    ret.x *= aspect;  // orig aspect ratio
    float newWidth = newSize.x * aspect;
    return ret + vec2(newTL.x - (newWidth - newSize.x) / 2., newTL.y);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv =
        getuv_centerX(fragCoord, vec2(-1, -1), vec2(2, 2));  // 0-1 centered

    vec3 col = vec3(1., 1., .875);  // bg

    // geometry on display...
    float yo = sin(-uv.x * pi * .5) * .2;
    col = colorBrushStrokeLine(uv, col,
                               vec4(vec3(.8, .1, 0), .9),  // red fixed line
                               vec2(-1.4, -.4 + yo), vec2(2.6, -.4 + yo), .3);

    fragColor = vec4(col, 1.);
}
