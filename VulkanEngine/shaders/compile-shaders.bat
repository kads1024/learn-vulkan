@echo off
glslc simple_shader.vert -o simple_shader.vert.spv
glslc simple_shader.frag -o simple_shader.frag.spv
echo Successfully compiled simple_shader.vert and simple_shader.frag
pause