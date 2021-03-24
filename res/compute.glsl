#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D img_output;
layout(binding = 1) uniform sampler2D u_texture;

uniform mat4 ProjectionViewMatrix;

layout(std430, binding = 0) buffer layoutName
{
    int blocks[16 * 16 * 16];
};

struct Ray
{
    vec3 Origin;
    vec3 Direction;
};

struct RayTraceResult
{
    bool Hit;
    ivec3 PrevLocation;
    ivec3 CurrentLocation;
    vec3 HitPos;
};

vec3 GetWorldPosition(float x, float y, float projectionZPos)
{
    mat4 inversedMat = inverse(ProjectionViewMatrix);

    float m_Width = 1280;
    float m_Height = 720;

    vec3 store = vec3((2.0 * x) / m_Width - 1.0, (2.0 * y) / m_Height - 1.0, projectionZPos * 2 - 1);

    vec4 proStore = inversedMat * vec4(store, 1.0);
    store.x = proStore.x;
    store.y = proStore.y;
    store.z = proStore.z;
    store *= 1.0f / proStore.w;
    return store;
}

Ray GetRay(float x, float y)
{
    vec3 click3d = GetWorldPosition(x, y, 0);
    vec3 dir = normalize(GetWorldPosition(x, y, 1) - click3d);
    return Ray(click3d, dir);
}

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

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

int GetIndex(int x, int y, int z)
{
    return (x * 16 + z) * 16 + y;
}

bool IsBlockHit(ivec3 location)
{
    float d = distance(ivec3(0,0,0), location);

    if(d > 10) {
        float val = noise(location * 0.1);

        if (val > 0.5) {
            return true;
        }
    }

    return false;
}

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38

RayTraceResult RayTraceBlocks(vec3 start, vec3 direction, float maxDistance)
{
    RayTraceResult result = RayTraceResult(false, ivec3(0), ivec3(0), vec3(0));

    float _bin_size = 1.0;

    vec3 ray_start = start;
    vec3 ray_end = start + direction * maxDistance;

    vec3 ray = normalize(ray_end-ray_start);

    ivec3 current_voxel = ivec3(floor(ray_start[0]/_bin_size), floor(ray_start[1]/_bin_size), floor(ray_start[2]/_bin_size));
    ivec3 last_voxel = ivec3(floor(ray_end[0]/_bin_size), floor(ray_end[1]/_bin_size), floor(ray_end[2]/_bin_size));

    int stepX = (ray[0] >= 0) ? 1:-1; // correct
    int stepY = (ray[1] >= 0) ? 1:-1; // correct
    int stepZ = (ray[2] >= 0) ? 1:-1; // correct

    // Distance along the ray to the next voxel border from the current position (tMaxX, tMaxY, tMaxZ).
    float next_voxel_boundary_x = (current_voxel[0]+stepX)*_bin_size; // correct
    float next_voxel_boundary_y = (current_voxel[1]+stepY)*_bin_size; // correct
    float next_voxel_boundary_z = (current_voxel[2]+stepZ)*_bin_size; // correct

    // tMaxX, tMaxY, tMaxZ -- distance until next intersection with voxel-border
    // the value of t at which the ray crosses the first vertical voxel boundary
    float tMaxX = (ray[0]!=0) ? (next_voxel_boundary_x - ray_start[0])/ray[0] : FLT_MIN; //
    float tMaxY = (ray[1]!=0) ? (next_voxel_boundary_y - ray_start[1])/ray[1] : FLT_MIN; //
    float tMaxZ = (ray[2]!=0) ? (next_voxel_boundary_z - ray_start[2])/ray[2] : FLT_MIN; //

    // tDeltaX, tDeltaY, tDeltaZ --
    // how far along the ray we must move for the horizontal component to equal the width of a voxel
    // the direction in which we traverse the grid
    // can only be FLT_MAX if we never go in that direction
    float tDeltaX = (ray[0]!=0) ? _bin_size/ray[0]*stepX : FLT_MIN;
    float tDeltaY = (ray[1]!=0) ? _bin_size/ray[1]*stepY : FLT_MIN;
    float tDeltaZ = (ray[2]!=0) ? _bin_size/ray[2]*stepZ : FLT_MIN;

    ivec3 diff = ivec3(0,0,0);

    bool neg_ray=false;
    if (current_voxel[0]!=last_voxel[0] && ray[0]<0) { diff[0]--; neg_ray=true; }
    if (current_voxel[1]!=last_voxel[1] && ray[1]<0) { diff[1]--; neg_ray=true; }
    if (current_voxel[2]!=last_voxel[2] && ray[2]<0) { diff[2]--; neg_ray=true; }

    if (neg_ray) {
        current_voxel += diff;
    }

    ivec3 prevLoc = current_voxel;

    int iterCount = 0;

    while(last_voxel != current_voxel) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                current_voxel[0] += stepX;
                tMaxX += tDeltaX;
            } else {
                current_voxel[2] += stepZ;
                tMaxZ += tDeltaZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                current_voxel[1] += stepY;
                tMaxY += tDeltaY;
            } else {
                current_voxel[2] += stepZ;
                tMaxZ += tDeltaZ;
            }
        }

        if(IsBlockHit(current_voxel)) {
            result.Hit = true;
            result.PrevLocation = prevLoc;
            result.CurrentLocation = current_voxel;
            result.HitPos = vec3(tMaxX, tMaxY, tMaxZ);
            result.HitPos = abs(result.HitPos);
            return result;
        }

        prevLoc = current_voxel;
        iterCount++;

        if(iterCount > 10000) break;
    }

    return result;
}

vec3 GetSkyColor(Ray ray, vec3 dirLight);

void main() {
    // base pixel colour for image
    vec4 pixel = vec4(1.0, 0.0, 0.0, 1.0);
    // get index in global work group i.e x,y position
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    float max_x = 5.0;
    float max_y = 5.0;
    ivec2 dims = imageSize(img_output); // fetch image dimensions
    float x = (float(pixel_coords.x * 2 - dims.x) / dims.x);
    float y = (float(pixel_coords.y * 2 - dims.y) / dims.y);

    Ray ray = GetRay(pixel_coords.x, pixel_coords.y);

    vec3 skyColor = GetSkyColor(ray, vec3(0.0, 0, 0));
    pixel.rgb = skyColor;

    RayTraceResult val = RayTraceBlocks(ray.Origin, ray.Direction, 50);

    if(val.Hit) {
        vec3 normal = normalize(val.CurrentLocation - val.PrevLocation);

        float light = dot(normal, normalize(vec3(0.5, 0.5, 0.5)));
        light = clamp(light, 0.2, 1.0);

        vec2 uv = vec2(0.0);

        if(val.HitPos.x <= 0.01) {
            uv = val.HitPos.yz;
        } else if(val.HitPos.y <= 0.01) {
            uv = val.HitPos.xz;
        } else if(val.HitPos.z <= 0.01) {
            uv = val.HitPos.xy;
        }

        pixel.rgb = vec3(light);
        pixel.rgb = texture(u_texture, val.HitPos.xz).rgb;
        pixel.rgb = val.CurrentLocation / 100.0;
    }

    // output to a specific pixel in the image
    imageStore(img_output, pixel_coords, pixel);
}

vec3 RGB(float r, float g, float b)
{
    return vec3(r / 255.0, g / 255.0, b / 255.0);
}

vec3 GetSkyColor(Ray ray, vec3 dirLight)
{
    vec3 ray_world = ray.Direction;

    float lightDot = dot(dirLight, vec3(0, 1, 0));
    float lightDotNormalized = (lightDot + 1.0) / 2.0;

    // Day values
    vec3 DayTopColor = RGB(85, 105, 170);
    vec3 DayBottomColor = RGB(205, 200, 252)* 1.3;
    float DayStartBlend = -0.5;
    float DayEndBlend = 1.5;
    vec3 SunColorUp = RGB(255, 255, 255) * 0.8;
    vec3 SunColorDown = RGB(255, 255, 255) * 0.7;
    float SunStartBlend = 1;
    float SunEndBlend = 0.5;
    float SunSize = 10.0; // This value is inversed, the bigger the value is the smallest is the sun

    // NightValues
    vec3 NightTopColor = RGB(0, 10, 51) * 0.7;
    vec3 NightBottomColor = RGB(205, 220, 255) * 0.5;
    float NightStartBlend = -0.5;
    float NightEndBlend = 1.5;

    // Day blending
    vec3 day = mix(DayBottomColor, DayTopColor, smoothstep(DayStartBlend, DayEndBlend, ray_world.y + 0.5));
    float sundot = clamp(dot(ray_world,dirLight),0.0,1.0);
    vec3 sunColor = mix(SunColorDown, SunColorUp, 1.0 - smoothstep(SunStartBlend, SunEndBlend, lightDotNormalized));
    vec3 sun = sunColor * pow(sundot, SunSize);

    // NightBlending
    vec3 night = mix(NightBottomColor, NightTopColor, smoothstep(NightStartBlend, NightEndBlend, ray_world.y + 0.5));

    // Day/Night cycle blending
    //float cycleBlendValue = 1.0 - smoothstep(-0.5, -0.3, sunDot);
    float cycleBlendValue = 1.0 - lightDotNormalized;

    vec3 cycleColor = mix(day, night, cycleBlendValue);
    cycleColor = mix(cycleColor + sun, sun, clamp(pow(sundot, SunSize), 0, 1));

    vec4 FragColor = vec4(cycleColor, 1.0);

    // contrast
    // You can disable this if you want, just remove or comment this two lines
    FragColor.rgb = clamp(FragColor.rgb, 0., 1.);
    FragColor.rgb = FragColor.rgb*FragColor.rgb*(3.0-2.0*FragColor.rgb);


    // saturation (amplify colour, subtract grayscale)
    // You can disable this if you want, just remove or comment this two lines
    float sat = 0.2;
    FragColor.rgb = FragColor.rgb * (1. + sat) - sat*dot(FragColor.rgb, vec3(0.33));

    return FragColor.rgb;
}