#version 330 core

layout(points) in;
layout(points, max_vertices = 1) out;

in POINT 
{
  bool valid;
  vec4 point;
  uint label;
  uint visible;
  vec4 rgbacolor;
  vec2 scanindex;
} gs_in[];


out vec4  out_point;
out uint  out_label;
out uint  out_visible;
out vec4  out_rgbacolor;
out vec2  out_scanindex;


void main()
{
  if(gs_in[0].valid)
  {
    out_point = gs_in[0].point;
    out_label = gs_in[0].label;
    out_visible = gs_in[0].visible;
    out_rgbacolor = gs_in[0].rgbacolor;
    out_scanindex = gs_in[0].scanindex;

    EmitVertex();
    EndPrimitive();  
  }
  
}