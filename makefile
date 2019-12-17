
SRC = src
OBJ = src
OBJS=

ifndef TARGETOS
TARGETOS=linux
endif

ifndef USE_HIDAPI
USE_HIDAPI=1
endif

CXXDEFS=-DUSE_HID=$(USE_HIDAPI)

SOURCES = $(SRC)/msigd.cpp
TARGETS = msigd

CXX = g++
CXXFLAGS = -O2 -g -Wall -Wextra -std=c++14 $(CXXEXTRAFLAGS)
#CC = clang++
#CFLAGS = -O2 -g -Wall -Wextra -std=c++14 -Weverything -Wno-c++98-compat -Wno-weak-vtables

LD = $(CXX)

ifeq ($(TARGETOS),windows)
	LIBS = -lusb
	ifeq ($(USE_HIDAPI),1)
		LIBS += -lhidapi
	endif
	CXXEXTRAFLAGS += -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0501 -DWIN32_LEAN_AND_MEAN
	LDEXTRAFLAGS += -Wl,--subsystem,console municode 
	MD=@mkdir.exe 
	DOXYGEN=doxygen.exe
else 
ifeq ($(TARGETOS),osx)
	LIBS = -lusb
	ifeq ($(USE_HIDAPI),1)
		LIBS += -lhidapi
	endif
else
	LIBS = -lusb
	ifeq ($(USE_HIDAPI),1)
		LIBS += -lhidapi-hidraw
	endif
endif
endif


MAKEFILE_TARGETS_WITHOUT_INCLUDE := clean doc clang mingw nvcc


all:    depend $(TARGETS)

mingw:
	$(MAKE) CXXEXTRAFLAGS="-DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0501 \
		-DWIN32_LEAN_AND_MEAN" LDEXTRAFLAGS="-Wl,--subsystem,console \
		-municode" LIBS=-lusb MD=@mkdir.exe DOXYGEN=doxygen.exe
		
#-------------------------------------------------
# clean
#-------------------------------------------------

clean:
	$(RM) -rf $(OBJS) $(TARGETS) $(OBJ)/msigd.o .depend doxy/*

#-------------------------------------------------
# msigd
#-------------------------------------------------

msigd: $(OBJ)/msigd.o $(OBJS)
	@echo Linking $@...
	$(LD) -o $@ $(LDFLAGS) $^ $(LIBS)

#-------------------------------------------------
# depends
#-------------------------------------------------

.depend: $(SOURCES)
	@echo creating .depend
	@rm -f ./.depend
	@for i in $(SOURCES); do \
		$(CXX) $(CXXFLAGS) -MM $$i -MT `echo $$i | sed -e 's+$(SRC)+$(OBJ)+' -e 's+.cpp+.o+' ` >> ./.depend; \
	done

depend: .depend

# Include only if the goal needs it
ifeq ($(filter $(MAKECMDGOALS),$(MAKEFILE_TARGETS_WITHOUT_INCLUDE)),)
-include .depend
endif


#-------------------------------------------------
# generic rules
#-------------------------------------------------

$(OBJ)/%.o: $(SRC)/%.cpp
	@echo Compiling $<...
	@$(CXX) $(CXXDEFS) $(CXXFLAGS) -c $< -o $@

$(OBJ)/%.pp: $(SRC)/%.cpp
	@echo Compiling $<...
	@$(CXX) $(CXXDEFS) $(CXXFLAGS) -E $< -o $@

$(OBJ)/%.s: $(SRC)/%.cpp
	@echo Compiling $<...
	@$(CXX) $(CXXDEFS) $(CXXFLAGS) -S $< -o $@

$(OBJ)/%.a:
	@echo Archiving $@...
	$(RM) $@
	$(AR) $(ARFLAGS) $@ $^

$(OBJ)/%.json: $(SRC)/%.cpp
	@echo Building compile database entry for $< ...
	@echo { \"directory\": \".\", >> $(TIDY_DB)
	@echo   \"command\": \"$(CC) $(CXXDEFS) $(CXXFLAGS) -c $< -o dummy.o\", >> $(TIDY_DB)
	@echo   \"file\": \"$(CURDIR)/$<\" } >> $(TIDY_DB)
	@echo "," >> $(TIDY_DB)

compile_commands_prefix:
	@echo "[" > $(TIDY_DB)

compile_commands_postfix:
	@echo "]" >> $(TIDY_DB)

