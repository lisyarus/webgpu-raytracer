#!/usr/bin/env bash

echo "Total commits:" $(git rev-list --all --count)
echo "Source lines: " $(find include/ source/ -name '*.cpp' -or -name '*.hpp' | xargs cat | wc -l)
echo "Shader lines: " $(find shaders/  -name '*.wgsl' | xargs cat | wc -l)
