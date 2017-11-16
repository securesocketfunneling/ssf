#!/bin/sh

if [ $# -lt 3 ]; then
  echo "Usage: $0 openssl_archive openssl_version destination_dir" 1>&2
  exit 1
fi

OPENSSL_ARCHIVE=$(realpath $1)
OPENSSL_VERSION=$2
OPENSSL_SOURCE=openssl-${OPENSSL_VERSION}
DIST_DIR=$(realpath $3)

if [ ! -d ${OPENSSL_SOURCE} ]; then
  echo "[*] Decompressing ${OPENSSL_ARCHIVE}"
  gzip -d ${OPENSSL_ARCHIVE} -c | tar xv
fi

CONFIG_ARGS="--prefix=${DIST_DIR} no-shared"

if [ "${CROSS_PREFIX}" = "" ]; then
  (cd ${OPENSSL_SOURCE} && sh ./config ${CONFIG_ARGS})
else
  arch=$(${CROSS_PREFIX}g++ -dumpmachine | cut -d '-' -f 1)
  case "${arch}" in
    mips)
      os_comp=linux-mips32
      ;;
    arm)
      os_comp=linux-armv4
      ;;
    *)
      echo "Cross-compiling currently unsupported for architecture: ${arch}" 2>&1
      exit 1
      ;;
  esac
  (cd ${OPENSSL_SOURCE} && perl Configure --cross-compile-prefix=${CROSS_PREFIX} ${CONFIG_ARGS} ${os_comp})
fi

(cd ${OPENSSL_SOURCE} && make all install_sw)
