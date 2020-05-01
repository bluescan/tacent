.PHONY: clean All

All:
	@echo "----------Building project:[ Foundation - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/Foundation/Linux" && "$(MAKE)" -f  "Foundation.mk"
	@echo "----------Building project:[ Math - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/Math/Linux" && "$(MAKE)" -f  "Math.mk"
	@echo "----------Building project:[ System - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/System/Linux" && "$(MAKE)" -f  "System.mk"
	@echo "----------Building project:[ Image - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/Image/Linux" && "$(MAKE)" -f  "Image.mk"
	@echo "----------Building project:[ Scene - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/Scene/Linux" && "$(MAKE)" -f  "Scene.mk"
	@echo "----------Building project:[ UnitTests - Release ]----------"
	@"$(MAKE)" -f  "UnitTests.mk" && "$(MAKE)" -f  "UnitTests.mk" PostBuild
clean:
	@echo "----------Cleaning project:[ Foundation - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/Foundation/Linux" && "$(MAKE)" -f  "Foundation.mk"  clean
	@echo "----------Cleaning project:[ Math - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/Math/Linux" && "$(MAKE)" -f  "Math.mk"  clean
	@echo "----------Cleaning project:[ System - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/System/Linux" && "$(MAKE)" -f  "System.mk"  clean
	@echo "----------Cleaning project:[ Image - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/Image/Linux" && "$(MAKE)" -f  "Image.mk"  clean
	@echo "----------Cleaning project:[ Scene - Release ]----------"
	@cd "/home/tristan/github/tacit-texview/Tacent/Modules/Scene/Linux" && "$(MAKE)" -f  "Scene.mk"  clean
	@echo "----------Cleaning project:[ UnitTests - Release ]----------"
	@"$(MAKE)" -f  "UnitTests.mk" clean
