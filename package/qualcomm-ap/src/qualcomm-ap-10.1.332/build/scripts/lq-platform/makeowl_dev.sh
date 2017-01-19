#!/bin/bash

# source the variables needed for build
TOP=$1/build
cd $TOP

make common_fusion BOARD_TYPE=pb44fus BUILD_TYPE=jffs2 BUILD_CONFIG=_routing
make fusion_build BOARD_TYPE=pb44fus BUILD_TYPE=jffs2 BUILD_CONFIG=_routing

make common_fusion BOARD_TYPE=pb44fus BUILD_TYPE=jffs2
make fusion_build BOARD_TYPE=pb44fus BUILD_TYPE=jffs2

echo "---------------------"
echo "Resetting permissions"
echo "---------------------"
find . -name \* -user root -exec sudo chown build {} \; -print 
find . -name \.config  -exec chmod 777 {} \; -print 
