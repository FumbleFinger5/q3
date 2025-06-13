# Compiler, tools, and options
CC = gcc
CXX = g++
LINK = g++

# Use make r=1 for release version
ifdef r
CXXFLAGS = -pipe -O2 -Wall -Wextra -D_REENTRANT -fPIC $(DEFINES)
DEFINES = -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB
else
CXXFLAGS = -pipe -O0 -ggdb -Wall -Wextra -D_REENTRANT -fPIC $(DEFINES) -DDEBUG
DEFINES = -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DDEBUG
endif

# Include paths
INCPATH = -I. \
      -I/home/steve/Documents/prog/plib \
      -I/usr/include/x86_64-linux-gnu/qt5 \
      -I/usr/include/x86_64-linux-gnu/qt5/QtWidgets \
      -I/usr/include/x86_64-linux-gnu/qt5/QtGui \
      -I/usr/include/x86_64-linux-gnu/qt5/QtCore \
      -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++ \
      `pkg-config --cflags gtk+-3.0`

# Libraries
# Use pkg-config for Qt5 libraries for correct paths and linking flags
LIBS = -L/home/steve/Documents/prog/plib -lplib -lcurl -ljson-c \
       `pkg-config --libs Qt5Widgets Qt5Gui Qt5Core` \
       -lGL -lpthread \
       `pkg-config --libs gtk+-3.0`

# Output directory
OBJECTS_DIR = /home/steve/Documents/prog/q3temp/

# Sources and objects
SOURCES = main.cpp myqsettings.cpp window.cpp
MOC_HEADERS = window.h myqsettings.h  # List ALL headers here
MOC_SOURCES = $(MOC_HEADERS:%.h=$(OBJECTS_DIR)moc_%.cpp)
MOC_OBJECTS = $(MOC_SOURCES:$(OBJECTS_DIR)%.cpp=$(OBJECTS_DIR)%.o)
OBJECTS = $(addprefix $(OBJECTS_DIR), $(notdir $(SOURCES:.cpp=.o))) $(MOC_OBJECTS)

# Target
TARGET = q3

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LINK) $(OBJECTS) $(LIBS) -o $@

$(OBJECTS_DIR)%.o: %.cpp | $(OBJECTS_DIR)
	$(CXX) -c $(CXXFLAGS) $(DEFINES) $(INCPATH) -o $@ $<

$(OBJECTS_DIR)moc_%.o: $(OBJECTS_DIR)moc_%.cpp
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -I. $(INCPATH) -o $@ $<  # The -I. is crucial

#$(OBJECTS_DIR)moc_%.cpp: %.h | $(OBJECTS_DIR)
#	/usr/lib/qt5/bin/moc $(DEFINES) $(INCPATH) $< -o $@

$(OBJECTS_DIR)moc_%.cpp: %.h | $(OBJECTS_DIR)
	/usr/lib/qt5/bin/moc $(DEFINES) $(INCPATH) -p./ $< -o $@


$(OBJECTS_DIR):
	mkdir -p $(OBJECTS_DIR)

clean:
	rm -rf $(OBJECTS_DIR)
	rm -f $(TARGET)

.PHONY: all clean
