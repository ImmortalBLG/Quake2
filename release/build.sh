#/bin/bash

# set "single" to "1" (builtin ref_xxx) or "0" (external ref_xxx)
single=1

function CheckVC()
{
  if [ "$workpath" ]; then
    return  # already found
  fi
  if [ -f "$1/bin/nmake.exe" ]; then
    echo "Using Visual C++ found at \"$2\" ..."
    workpath="$1"
    workpath2="$2"
  fi
}

#------- Find VisualStudio on local drives and setup path variables -------
# check vc6
#CheckVC "/cygdrive/c/vc6" "c:/vc6"
CheckVC "/cygdrive/c/progra~1/msvs/vc98" "c:/progra~1/msvs/vc98"
CheckVC "/cygdrive/c/progra~1/micros~2/vc98" "c:/progra~1/micros~2/vc98"

if [ ! "$workpath" ]; then
  echo "ERROR: Visual C++ is not found."
  exit 1
fi

PATH="$workpath/bin:$workpath/shared~1/bin:$workpath/../common/msdev98/bin:$PATH"
typeset -x INCLUDE="$workpath2/INCLUDE;$workpath2/MFC/INCLUDE"
typeset -x LIB="$workpath2/LIB"
cd ..

echo "----- Building Quake2 -----"
if [ "$single" == "1" ]; then
  nmake /nologo /s /f "quake2s.mak" CFG="quake2s - Win32 Release" | tee release/build.log | vc32filt --filter
else
  nmake /nologo /s /f "quake2.mak" CFG="quake2 - Win32 Release" | tee release/build.log | vc32filt --filter
fi

if ! [ ${PIPESTATUS[0]} == "0" ]; then
  exit 1;
fi

echo "----- Building old ref_gl -----"
cd ref_gl.old
nmake /nologo /s /f "ref_gl.mak" CFG="ref_gl - Win32 Release" | tee release/build.log | vc32filt --filter
cd ..

echo "Build done."