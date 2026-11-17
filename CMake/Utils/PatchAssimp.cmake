# Argon: patches applied to the Assimp sources that Dependencies.cmake downloads.
# Usage: cmake -DASSIMP_SOURCE_DIR=<path to assimp-x.y.z> -P PatchAssimp.cmake
# Each patch is idempotent and fails loudly if its anchor is missing (new Assimp version).

if(NOT ASSIMP_SOURCE_DIR)
  message(FATAL_ERROR "PatchAssimp.cmake: ASSIMP_SOURCE_DIR is not set")
endif()

# FBX Euler rotation curves that jump across +-180 degrees between two keys (177 -> -179) are
# the same orientation reached by a short turn, but GetRotationKeyframeList inserted keys every
# 179 degrees to reproduce the long turn, so bones spun almost a full revolution between keys.
# Unwrapping each key to within 180 degrees of the previous one keeps every key's orientation
# and leaves no long jump to subdivide: the rotation between keys always takes the shortest path.
set(FBX_CONVERTER "${ASSIMP_SOURCE_DIR}/code/AssetLib/FBX/FBXConverter.cpp")
set(ROTATION_ANCHOR "while (std::abs(vc - vp) >= 180.0f) {")
set(ROTATION_PATCHED "vc -= 360.0f * std::round((vc - vp) / 360.0f); // Argon: shortest rotation path\n                    while (std::abs(vc - vp) > 180.0f) {")

file(READ "${FBX_CONVERTER}" FBX_CONVERTER_SOURCE)
string(FIND "${FBX_CONVERTER_SOURCE}" "// Argon: shortest rotation path" PATCHED_POSITION)
if(PATCHED_POSITION EQUAL -1)
  string(FIND "${FBX_CONVERTER_SOURCE}" "${ROTATION_ANCHOR}" ANCHOR_POSITION)
  if(ANCHOR_POSITION EQUAL -1)
    message(FATAL_ERROR "PatchAssimp.cmake: anchor for the FBX shortest rotation path patch not found in ${FBX_CONVERTER}")
  endif()
  string(REPLACE "${ROTATION_ANCHOR}" "${ROTATION_PATCHED}" FBX_CONVERTER_SOURCE "${FBX_CONVERTER_SOURCE}")
  file(WRITE "${FBX_CONVERTER}" "${FBX_CONVERTER_SOURCE}")
  message(STATUS "Patched Assimp FBX rotations to take the shortest path")
endif()
