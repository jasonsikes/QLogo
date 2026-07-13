# Prebuilt LLVM Windows packages embed an absolute path to diaguids.lib from the
# machine that built LLVM. Patch LLVMDebugInfoPDB to use the local DIA SDK.
function(fix_llvm_windows_dia_sdk)
  if(NOT MSVC OR NOT TARGET LLVMDebugInfoPDB)
    return()
  endif()

  if(CMAKE_GENERATOR_PLATFORM MATCHES "ARM64|arm64"
      OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64"
      OR CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "ARM64")
    set(_dia_arch arm64)
  elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM"
      OR CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM")
    set(_dia_arch arm)
  elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "x64"
      OR CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_dia_arch amd64)
  else()
    set(_dia_arch "")
  endif()

  set(_dia_guids_lib "")
  set(_dia_search_dirs)

  if(DEFINED ENV{VSINSTALLDIR})
    list(APPEND _dia_search_dirs "$ENV{VSINSTALLDIR}/DIA SDK/lib")
  endif()

  if(CMAKE_GENERATOR_INSTANCE)
    list(APPEND _dia_search_dirs "${CMAKE_GENERATOR_INSTANCE}/DIA SDK/lib")
  endif()

  set(_programfiles_x86 "ProgramFiles(x86)")
  set(_vswhere "$ENV{${_programfiles_x86}}/Microsoft Visual Studio/Installer/vswhere.exe")
  if(EXISTS "${_vswhere}")
    execute_process(
      COMMAND "${_vswhere}" -latest -products * -property installationPath
      OUTPUT_VARIABLE _vs_install
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    if(_vs_install)
      list(APPEND _dia_search_dirs "${_vs_install}/DIA SDK/lib")
    endif()
  endif()

  list(APPEND _dia_search_dirs
    "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/DIA SDK/lib"
    "C:/Program Files/Microsoft Visual Studio/2022/Professional/DIA SDK/lib"
    "C:/Program Files/Microsoft Visual Studio/2022/Community/DIA SDK/lib"
    "C:/Program Files/Microsoft Visual Studio/2022/BuildTools/DIA SDK/lib"
    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/DIA SDK/lib"
    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib"
    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/DIA SDK/lib"
    "C:/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/DIA SDK/lib"
  )

  if(_dia_arch)
    foreach(_base IN LISTS _dia_search_dirs)
      if(EXISTS "${_base}/${_dia_arch}/diaguids.lib")
        set(_dia_guids_lib "${_base}/${_dia_arch}/diaguids.lib")
        break()
      endif()
    endforeach()
  endif()

  if(NOT _dia_guids_lib)
    foreach(_base IN LISTS _dia_search_dirs)
      if(EXISTS "${_base}/diaguids.lib")
        set(_dia_guids_lib "${_base}/diaguids.lib")
        break()
      endif()
    endforeach()
  endif()

  if(NOT _dia_guids_lib)
    message(WARNING
      "diaguids.lib not found for ${_dia_arch}. LLVM's Windows packages require "
      "the Visual Studio DIA SDK (installed with the 'Desktop development with "
      "C++' workload). Install or repair Visual Studio, then reconfigure.")
    return()
  endif()

  get_target_property(_pdb_iface_libs LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)
  if(NOT _pdb_iface_libs OR _pdb_iface_libs STREQUAL "_pdb_iface_libs-NOTFOUND")
    return()
  endif()

  set(_fixed_libs)
  foreach(_lib IN LISTS _pdb_iface_libs)
    if(_lib MATCHES "diaguids\\.lib$")
      list(APPEND _fixed_libs "${_dia_guids_lib}")
    else()
      list(APPEND _fixed_libs "${_lib}")
    endif()
  endforeach()

  set_target_properties(LLVMDebugInfoPDB PROPERTIES
    INTERFACE_LINK_LIBRARIES "${_fixed_libs}"
  )
  message(STATUS "Using local DIA SDK library: ${_dia_guids_lib}")
endfunction()

