#!/bin/bash
#
# This script generates an archive release
# Includes :
#   - SSF binaries
#   - UPXed SSF binaries
#   - tests certs (for test purpose only)
#

set -e

echo "Usage: ./generate_release.sh SSF_VERSION CMAKE_GENERATOR ABS_SSF_SOURCE_DIR OUTPUT_FILEPATH"

if [ -z "$1" ]; then "First arg is VERSION"; exit 1; else VERSION="$1"; fi
if [ -z "$2" ]; then "Second arg is CMAKE_GENERATOR"; exit 1; else CMAKE_GENERATOR="$2"; fi
if [ -z "$3" ]; then "Third arg is ABS_SSF_SOURCE_DIR"; exit 1; else ABS_SSF_SOURCE_DIR="$3"; fi
if [ -z "$4" ]; then "Fourth arg is OUTPUT_FILE"; exit 1; else OUTPUT_FILEPATH="$4"; fi

PWD=`pwd`

TMP_BUILD_DIR="${PWD}/tmp_build_release"
TARGET="ssf-${VERSION}"
INSTALL_BIN_PATH="${TMP_BUILD_DIR}/ssf/${TARGET}"

echo "* VERSION: '${VERSION}'"
echo "* CMAKE_GENERATOR: '${CMAKE_GENERATOR}'"
echo "* ABS_SSF_SOURCE_DIR: '${ABS_SSF_SOURCE_DIR}'"
echo "* OUTPUT_FILEPATH: '${OUTPUT_FILEPATH}'"

echo "* Create tmp build directory '${TMP_BUILD_DIR}'"
mkdir -p ${TMP_BUILD_DIR}

cd ${TMP_BUILD_DIR}
echo "* CMake pre processing"
cmake ${ABS_SSF_SOURCE_DIR} -DCMAKE_BUILD_TYPE=Release -G "${CMAKE_GENERATOR}"

echo "* Build binaries"
cmake --build . --target install --config Release

echo "* UPX binaries"
for BIN_PATH in ${INSTALL_BIN_PATH}/ssf*;
do
  BIN_NAME=`basename ${BIN_PATH}`
  upx --best -o "${INSTALL_BIN_PATH}/upx-${BIN_NAME}" ${BIN_PATH}
done

echo "* Install directory '${INSTALL_BIN_PATH}'"
cd "${INSTALL_BIN_PATH}/.."

echo "* Create tar archive '${OUTPUT_FILEPATH}'"
tar -czf "${OUTPUT_FILEPATH}" "${TARGET}"

echo "* Clean tmp build directory '${TMP_BUILD_DIR}'"
rm -rf "${TMP_BUILD_DIR}"
