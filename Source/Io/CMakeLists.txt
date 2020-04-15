#
# Copyright (c) 2008-2019 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Define target name
set (TARGET_NAME Io)

# tolua library
include_directories (../ ../ThirdParty/toluapp/include/)

# imgui library
include_directories (../ThirdParty/imgui)
include_directories (../ThirdParty/imgui/examples/)

# Find Urho3D
find_package (Urho3D REQUIRED)
include_directories (${URHO3D_INCLUDE_DIRS})

define_source_files (EXTRA_CPP_FILES ../ThirdParty/imgui/imgui.cpp ../ThirdParty/imgui/imgui_draw.cpp
    ../ThirdParty/imgui/imgui_widgets.cpp ../ThirdParty/imgui/imgui_demo.cpp
    ../ThirdParty/imgui/examples/imgui_impl_dx11.cpp)

setup_main_executable ()

setup_test ()
