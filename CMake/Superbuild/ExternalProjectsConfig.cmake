##############################################################################
#
# Library:   TubeTK
#
# Copyright 2010 Kitware Inc. 28 Corporate Drive,
# Clifton Park, NY, 12065, USA.
#
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
##############################################################################

# See https://github.com/TubeTK/TubeTK/wiki/Dependencies

# Cppcheck version 1.61
set( Cppcheck_URL
  ${github_protocol}://github.com/danmar/cppcheck.git )
set( Cppcheck_HASH_OR_TAG 1.61 )

# Common Toolkit snapshot 2014-01-03
set( CTK_URL ${github_protocol}://github.com/commontk/CTK.git )
set( CTK_HASH_OR_TAG 8bebbc92df4b310c150e6a017b6b8966b4625089 )

# TubeTK Image Viewer snapshot 2014-01-26
set( ImageViewer_URL
  ${github_protocol}://github.com/TubeTK/TubeTK-ImageViewer.git )
set( ImageViewer_HASH_OR_TAG ef5e163c7d3f417a4d6180c5d12a5a06b462ad4c )

# Insight Segmentation and Registration Toolkit
#set( ITK_URL ${github_protocol}://github.com/Kitware/ITK.git )
#set( ITK_HASH_OR_TAG 9fb84edbf9f6a5bb0846acbf6ecd767ca5cab2e1 )
set( ITK_URL ${github_protocol}://itk.org/ITK.git )
set( ITK_HASH_OR_TAG 754278cb7ab50ae3e64dc0a413aef4b9ff0ff1f0 )

# JsonCpp snapshot 2014-01-01 r287
# ${svn_protocol}://svn.code.sf.net/p/jsoncpp/code/trunk/jsoncpp )
set( JsonCpp_URL http://midas3.kitware.com/midas/download/bitstream/337068/JsonCpp_r275.tar.gz )
set( JsonCpp_HASH_OR_TAG 43301ad1118004fbdc69b6f7b14a4cd5 )

# KWStyle snapshot 2012-04-19 04:05:19
set( KWStyle_URL
  ${git_protocol}://github.com/Kitware/KWStyle.git )
set( KWStyle_HASH_OR_TAG 9711cdbd35af37a9abcdd8b1dbd8e2b5a4ac8779 )

# LIBSVM version 3.17 (minimum version 3.1)
set( LIBSVM_URL
  http://www.csie.ntu.edu.tw/~cjlin/libsvm/oldfiles/libsvm-3.17.tar.gz )
set( LIBSVM_HASH_OR_TAG 67f8b597ce85c1f5288d7838e57ea28a )

# TubeTK Parameter Serializer snapshot 2014-02-05 )
set( ParameterSerializer_URL
  ${github_protocol}://github.com/TubeTK/TubeTK-ParameterSerializer.git )
set( ParameterSerializer_HASH_OR_TAG fe360ce153bfe4efc36b276750d4606bb1d7b156 )

# Slicer Execution Model snapshot 2014-02-18
set( SlicerExecutionModel_URL
  ${github_protocol}://github.com/Slicer/SlicerExecutionModel.git )
set( SlicerExecutionModel_HASH_OR_TAG 46f3d892fc6deb9be2ce929257a5411d02c1ae75 )

# Visualization Toolkit (3D Slicer fork) snapshot 2013-08-20 06:54:45
set( VTK_URL ${github_protocol}://github.com/Slicer/VTK.git )
set( VTK_HASH_OR_TAG c88dfedb277969e5f1f6c5349d8f7898610e75f4 )
