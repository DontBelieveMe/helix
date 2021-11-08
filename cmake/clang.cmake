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
		LLVMAsmParser.lib
		LLVMAsmPrinter.lib
		LLVMBinaryFormat.lib
		LLVMBitReader.lib
		LLVMBitstreamReader.lib
		LLVMBitWriter.lib
		LLVMCFGuard.lib
		LLVMCodeGen.lib
		LLVMCore.lib
		LLVMCoverage.lib
		LLVMDebugInfoCodeView.lib
		LLVMDebugInfoDWARF.lib
		LLVMDebugInfoMSF.lib
		LLVMDemangle.lib
		LLVMExtensions.lib
		LLVMFrontendOpenMP.lib
		LLVMGlobalISel.lib
		LLVMInstCombine.lib
		LLVMInstrumentation.lib
		LLVMIRReader.lib
		LLVMLinker.lib
		LLVMMC.lib
		LLVMMCDisassembler.lib
		LLVMMCParser.lib
		LLVMObjCARCOpts.lib
		LLVMObject.lib
		LLVMOption.lib
		LLVMProfileData.lib
		LLVMRemarks.lib
		LLVMScalarOpts.lib
		LLVMSelectionDAG.lib
		LLVMSupport.lib
		LLVMTableGen.lib
		LLVMTableGenGlobalISel.lib
		LLVMTarget.lib
		LLVMTextAPI.lib
		LLVMTransformUtils.lib
		LLVMVectorize.lib
		LLVMX86AsmParser.lib
		LLVMX86CodeGen.lib
		LLVMX86Desc.lib
		LLVMX86Disassembler.lib
		LLVMX86Info.lib
)

