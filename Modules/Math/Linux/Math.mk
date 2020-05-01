##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=Math
ConfigurationName      :=Debug
WorkspacePath          :=/home/tristan/github/tacent/UnitTests/Linux
ProjectPath            :=/home/tristan/github/tacent/Modules/Math/Linux
IntermediateDirectory  :=$(ConfigurationName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Tristan
Date                   :=30/04/20
CodeLitePath           :=/home/tristan/.codelite
LinkerName             :=/usr/bin/clang++
SharedObjectLinkerName :=/usr/bin/clang++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/lib$(ProjectName).a
Preprocessors          :=$(PreprocessorSwitch)PLATFORM_LINUX $(PreprocessorSwitch)CONFIG_DEBUG 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="Math.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch)../Inc $(IncludeSwitch)../../Foundation/Inc $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/llvm-ar rcu
CXX      := /usr/bin/clang++
CC       := /usr/bin/clang
CXXFLAGS := -Wno-switch -std=c++17 -g $(Preprocessors)
CFLAGS   :=  -g $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/llvm-as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/up_Src_tGeometry.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_Src_tSpline.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_Src_tHash.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_Src_tRandom.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_Src_tLinearAlgebra.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_Src_tColour.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(IntermediateDirectory) $(OutputFile)

$(OutputFile): $(Objects)
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(AR) $(ArchiveOutputSwitch)$(OutputFile) @$(ObjectsFileList)
	@$(MakeDirCommand) "/home/tristan/github/tacent/UnitTests/Linux/.build-debug"
	@echo rebuilt > "/home/tristan/github/tacent/UnitTests/Linux/.build-debug/Math"

MakeIntermediateDirs:
	@test -d $(ConfigurationName) || $(MakeDirCommand) $(ConfigurationName)


$(ConfigurationName):
	@test -d $(ConfigurationName) || $(MakeDirCommand) $(ConfigurationName)

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/up_Src_tGeometry.cpp$(ObjectSuffix): ../Src/tGeometry.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_Src_tGeometry.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_Src_tGeometry.cpp$(DependSuffix) -MM ../Src/tGeometry.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tristan/github/tacent/Modules/Math/Src/tGeometry.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_Src_tGeometry.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_Src_tGeometry.cpp$(PreprocessSuffix): ../Src/tGeometry.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_Src_tGeometry.cpp$(PreprocessSuffix) ../Src/tGeometry.cpp

$(IntermediateDirectory)/up_Src_tSpline.cpp$(ObjectSuffix): ../Src/tSpline.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_Src_tSpline.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_Src_tSpline.cpp$(DependSuffix) -MM ../Src/tSpline.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tristan/github/tacent/Modules/Math/Src/tSpline.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_Src_tSpline.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_Src_tSpline.cpp$(PreprocessSuffix): ../Src/tSpline.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_Src_tSpline.cpp$(PreprocessSuffix) ../Src/tSpline.cpp

$(IntermediateDirectory)/up_Src_tHash.cpp$(ObjectSuffix): ../Src/tHash.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_Src_tHash.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_Src_tHash.cpp$(DependSuffix) -MM ../Src/tHash.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tristan/github/tacent/Modules/Math/Src/tHash.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_Src_tHash.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_Src_tHash.cpp$(PreprocessSuffix): ../Src/tHash.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_Src_tHash.cpp$(PreprocessSuffix) ../Src/tHash.cpp

$(IntermediateDirectory)/up_Src_tRandom.cpp$(ObjectSuffix): ../Src/tRandom.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_Src_tRandom.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_Src_tRandom.cpp$(DependSuffix) -MM ../Src/tRandom.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tristan/github/tacent/Modules/Math/Src/tRandom.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_Src_tRandom.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_Src_tRandom.cpp$(PreprocessSuffix): ../Src/tRandom.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_Src_tRandom.cpp$(PreprocessSuffix) ../Src/tRandom.cpp

$(IntermediateDirectory)/up_Src_tLinearAlgebra.cpp$(ObjectSuffix): ../Src/tLinearAlgebra.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_Src_tLinearAlgebra.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_Src_tLinearAlgebra.cpp$(DependSuffix) -MM ../Src/tLinearAlgebra.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tristan/github/tacent/Modules/Math/Src/tLinearAlgebra.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_Src_tLinearAlgebra.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_Src_tLinearAlgebra.cpp$(PreprocessSuffix): ../Src/tLinearAlgebra.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_Src_tLinearAlgebra.cpp$(PreprocessSuffix) ../Src/tLinearAlgebra.cpp

$(IntermediateDirectory)/up_Src_tColour.cpp$(ObjectSuffix): ../Src/tColour.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_Src_tColour.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_Src_tColour.cpp$(DependSuffix) -MM ../Src/tColour.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tristan/github/tacent/Modules/Math/Src/tColour.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_Src_tColour.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_Src_tColour.cpp$(PreprocessSuffix): ../Src/tColour.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_Src_tColour.cpp$(PreprocessSuffix) ../Src/tColour.cpp


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r $(ConfigurationName)/


