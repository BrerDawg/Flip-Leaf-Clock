# A Makefile for both Linux and Windows, 06-dec-2017

#define all executables here
app_name= flip_leaf_clock


all: ${app_name}


#define compiler options	
CC=g++

ifneq ($(OS),Windows_NT)			#linux?
	CFLAGS=-g -O3 -Wfatal-errors -Wfatal-errors -fpermissive -Dbuild_date="\"`date +%Y-%b-%d`\"" #-Dbuild_date="\"2016-Mar-23\""			#64 bit
	LIBS= -lX11 -lrt -lm -lXfixes -lXext -lXft #64 bit
	INCLUDE= -I/usr/include/freetype2	#64 bit

else								#windows?
#	CFLAGS=-g -DWIN32 -mms-bitfields -Dcompile_for_windows -Dbuild_date="\"`date +%Y\ %b\ %d`\""
#LIBS= -L/usr/local/lib -static -mwindows -lfltk_images -lfltk -lfltk_png -lfltk_jpeg -lole32 -luuid -lcomctl32 -lwsock32 -lWs2_32 -lm -lfftw3 -lwinmm
#	INCLUDE= -I/usr/local/include
endif



#define object files for each executable, see dependancy list at bottom
obj1= flip_leaf_clock.o GCProfile.o texturewrap_code.o 
#obj2= backprop.o layer.o



#linker definition
flip_leaf_clock: $(obj1)
	$(CC) $(CFLAGS) -o $@ $(obj1) $(LIBS)




#compile definition for all cpp files to be complied into .o files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

%.o: %.cxx
	$(CC) $(CFLAGS) $(INCLUDE) -c $<



#dependancy list per each .o file
flip_leaf_clock.o: flip_leaf_clock.h GCProfile.h texturewrap_code.h
GCProfile.o: GCProfile.h
texturewrap_code.o: texturewrap_code.h GCProfile.h


.PHONY : clean
clean : 
		-rm $(obj1)					#remove obj files
ifneq ($(OS),Windows_NT)
		-rm ${app_name}				#remove linux exec
else
		-rm ${app_name}.exe			#remove windows exec
endif


