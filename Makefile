include makefile.in

INCLUDE = -I$(OPENMESH_INCLUDE_DIR) -Iinclude/ -I$(EIGEN_DIR)
CPPFLAGS = -O3 -fPIC -DEIGEN_PERMANENTLY_DISABLE_STUPID_WARNINGS -DEIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET 
LDFLAGS = -O3 -framework OpenGL -framework GLUT
LIB = -lOpenMeshCore -lOpenMeshTools -lsfml-graphics -lsfml-window -Wl,-rpath,$(OPENMESH_LIB_DIR)
TARGET = drawMesh
OBJS = objs/main.o objs/curvature.o objs/mesh_features.o objs/image_generation.o objs/decimate.o objs/shader.o

default: $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -L$(OPENMESH_LIB_DIR) $(LIB) -o $(TARGET)
	
objs/main.o: src/main.cpp
	$(CPP) -c $(CPPFLAGS) src/main.cpp -o objs/main.o $(INCLUDE)

objs/curvature.o: src/curvature.cpp
	$(CPP) -c $(CPPFLAGS) src/curvature.cpp -o objs/curvature.o $(INCLUDE)

objs/mesh_features.o: src/mesh_features.cpp
	$(CPP) -c $(CPPFLAGS) src/mesh_features.cpp -o objs/mesh_features.o $(INCLUDE)

objs/image_generation.o: src/image_generation.cpp
	$(CPP) -c $(CPPFLAGS) src/image_generation.cpp -o objs/image_generation.o $(INCLUDE)

objs/decimate.o: src/decimate.cpp
	$(CPP) -c $(CPPFLAGS) src/decimate.cpp -o objs/decimate.o $(INCLUDE)

objs/shader.o: src/shader.cpp
	$(CPP) -c $(CPPFLAGS) src/shader.cpp -o objs/shader.o $(INCLUDE)

clean:
	rm -f $(OBJS) $(TARGET)
