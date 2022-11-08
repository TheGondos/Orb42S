# Copyright (c) 2022 Gondos
# Licensed under the GPLv3 License or later

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DBCD_MATH")

add_library(Orb42S SHARED
	Orb42S.cpp
	plus42desktop/common/core_commands1.cc
	plus42desktop/common/core_commands5.cc
	plus42desktop/common/core_commands9.cc
	plus42desktop/common/core_globals.cc 
	plus42desktop/common/core_linalg2.cc  
	plus42desktop/common/core_parser.cc  
	plus42desktop/common/core_variables.cc 
	plus42desktop/common/core_commands2.cc 
	plus42desktop/common/core_commands6.cc 
	plus42desktop/common/core_commandsa.cc 
	plus42desktop/common/core_helpers.cc  
	plus42desktop/common/core_main.cc    
	plus42desktop/common/core_phloat.cc  
	plus42desktop/common/core_commands3.cc 
	plus42desktop/common/core_commands7.cc 
	plus42desktop/common/core_display.cc   
	plus42desktop/common/core_keydown.cc  
	plus42desktop/common/core_math1.cc   
	plus42desktop/common/core_sto_rcl.cc 
	plus42desktop/common/core_commands4.cc 
	plus42desktop/common/core_commands8.cc  
	plus42desktop/common/core_equations.cc 
	plus42desktop/common/core_linalg1.cc  
	plus42desktop/common/core_math2.cc   
	plus42desktop/common/core_tables.cc 
)

target_include_directories(Orb42S
	PUBLIC ${ORBITER_BINARY_SDK_DIR}/include
	plus42desktop/common
)

target_link_libraries(Orb42S
	${ORBITER_SDK_LIB}
)

set_target_properties(Orb42S
	PROPERTIES
	FOLDER Modules
)

add_dependencies(Orb42S
	${OrbiterTgt}
	Orbitersdk
)

# Installation
install(TARGETS Orb42S
	#RUNTIME
	DESTINATION ${ORBITER_INSTALL_PLUGIN_DIR}
)
