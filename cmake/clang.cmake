link_directories(${CMAKE_SOURCE_DIR}/lib/bin)

set(llvm ${CMAKE_SOURCE_DIR}/external/llvm-project)

include_directories(
	${llvm}/llvm/include
	${llvm}/clang/include
)

include_directories(${CMAKE_SOURCE_DIR}/lib/include)

add_library(Clang::LibTooling INTERFACE IMPORTED)

if(WIN32)
	target_link_libraries(Clang::LibTooling INTERFACE Version.lib)
endif()

target_link_libraries(Clang::LibTooling INTERFACE
		clangAnalysis.lib
		clangAST.lib
		clangASTMatchers.lib
		clangBasic.lib
		clangDriver.lib
		clangEdit.lib
		clangFrontend.lib
		clangLex.lib
		clangParse.lib
		clangRewrite.lib
		clangSema.lib
		clangSerialization.lib
		clangTooling.lib
		clangToolingCore.lib
		clangToolingSyntax.lib
		LLVMAggressiveInstCombine.lib
		LLVMAnalysis.lib
		LLVMBinaryFormat.lib
		LLVMBitReader.lib
		LLVMBitstreamReader.lib
		LLVMCore.lib
		LLVMDebugInfoCodeView.lib
		LLVMDemangle.lib
		LLVMFrontendOpenMP.lib
		LLVMInstCombine.lib
		LLVMMC.lib
		LLVMMCParser.lib
		LLVMObject.lib
		LLVMOption.lib
		LLVMProfileData.lib
		LLVMRemarks.lib
		LLVMScalarOpts.lib
		LLVMSupport.lib
		LLVMTableGen.lib
		LLVMTableGenGlobalISel.lib
		LLVMTextAPI.lib
		LLVMTransformUtils.lib
)

