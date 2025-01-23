@echo off
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
echo Successfully compiled shader.vert and shader.frag
pause