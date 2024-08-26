struct VertexInput {
    @location(0) position: vec2f,
    @location(1) color: vec3f
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f
};

@group(0) @binding(0) var<uniform> uTime: f32;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    let ratio = 800.0 / 600.0;
    var offset = vec2f(-0.6874, -0.463);
    offset += vec2f(cos(uTime), sin(uTime));
    let pos = vec4f(in.position.x + offset.x, in.position.y * ratio + offset.y, 0.0, 1.0);
    return VertexOutput(pos, in.color);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.color, 1.0);
}