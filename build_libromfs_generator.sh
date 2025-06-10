set -e

# ======== Manually building libromfs generator ========
# This project supports using libromfs to package the main program and resource files together to avoid additional file dependencies
# The function of libromfs-generator is to convert resource files into cpp source code and link them into the main program
# There are two solutions for cross-platform compilation:
# 1. Pre-compile libromfs-generator in the format of the host machine.
# 2. Set CMAKE_CROSSCOMPILING_EMULATOR to simulate execution libromfs-generator on the host.
# This script is used to generate libromfs-generator in the format of the host machine.

echo "Build libromfs-generator"

PROJECT_PATH=$(dirname "$0")
LIBROMFS_PATH="${PROJECT_PATH}/library/borealis/library/lib/extern/libromfs/generator"
BUILD_DIR="build_libromfs_generator"

cd "${PROJECT_PATH}"

# build libromfs-generator
cmake -B ${BUILD_DIR} "${LIBROMFS_PATH}"
make -C ${BUILD_DIR}

# put libromfs-generator under the jni folder
cp ${BUILD_DIR}/libromfs-generator "${PROJECT_PATH}"
echo "Build libromfs-generator: ${PROJECT_PATH}/libromfs-generator"

# remove build folder
rm -rf ${BUILD_DIR}
echo "Remove temp build dir: ${BUILD_DIR}"
