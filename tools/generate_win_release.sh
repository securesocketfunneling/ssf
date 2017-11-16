#!/bin/bash
#
# This script generates a Windows release
# Includes:
#   - SSF binaries
#   - UPXed SSF binaries
#   - tests certs (for test purpose only)

set -e

echo "Usage: ./generate_win_release.sh SSF_VERSION CMAKE_GENERATOR ABS_SSF_SOURCE_DIR BOOST_PATH OPENSSL_PATH BUILD_TYPE BUILD_DIR"

if [ -z "$1" ]; then "Missing VERSION"; exit 1; else VERSION="$1"; fi
if [ -z "$2" ]; then "Missing CMAKE_GENERATOR"; exit 1; else CMAKE_GENERATOR="$2"; fi
if [ -z "$3" ]; then "Missing ABS_SSF_SOURCE_DIR"; exit 1; else ABS_SSF_SOURCE_DIR="$3"; fi
if [ -z "$4" ]; then "Missing BOOST_PATH"; exit 1; else BOOST_PATH="$4"; fi
if [ -z "$5" ]; then "Missing OPENSSL_PATH"; exit 1; else OPENSSL_PATH="$5"; fi
if [ -z "$6" ]; then "Missing BUILD_TYPE"; exit 1; else BUILD_TYPE="$6"; fi
if [ -z "$7" ]; then "Missing BUILD_DIR"; exit 1; else BUILD_DIR="$7"; fi

TARGET="ssf-${VERSION}"
INSTALL_BIN_PATH="${BUILD_DIR}/ssf/${TARGET}"

echo "* VERSION: '${VERSION}'"
echo "* CMAKE_GENERATOR: '${CMAKE_GENERATOR}'"
echo "* ABS_SSF_SOURCE_DIR: '${ABS_SSF_SOURCE_DIR}'"
echo "* OUTPUT_FILEPATH: '${OUTPUT_FILEPATH}'"
echo "* BOOST_PATH: '${BOOST_PATH}'"
echo "* OPENSSL_PATH: '${OPENSSL_PATH}'"
echo "* BUILD_TYPE: '${BUILD_TYPE}'"

echo "* Create build directory '${BUILD_DIR}'..."
mkdir -p ${BUILD_DIR}

cd ${BUILD_DIR}
echo "* CMake pre processing..."
cmake ${ABS_SSF_SOURCE_DIR} -G "${CMAKE_GENERATOR}" -T v141_xp -DUSE_STATIC_LIBS=ON -DDISABLE_RTTI=ON -DBOOST_ROOT=$BOOST_PATH -DOPENSSL_ROOT_DIR=$OPENSSL_PATH -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX=${INSTALL_BIN_PATH}

echo "* Build binaries..."
cmake --build . --target install --config $BUILD_TYPE -- -maxcpucount

mv ${INSTALL_BIN_PATH}/bin/* ${INSTALL_BIN_PATH}
rmdir ${INSTALL_BIN_PATH}/bin

echo "* UPX binaries..."
for BIN_PATH in ${INSTALL_BIN_PATH}/ssf*;
do
  BIN_NAME=`basename ${BIN_PATH}`
  upx --best -o "${INSTALL_BIN_PATH}/upx-${BIN_NAME}" ${BIN_PATH}
done

cd -

echo "* Release directory: '${INSTALL_BIN_PATH}'"
